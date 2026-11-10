#include "IO_epoll_Test.h"

#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <unordered_map>

namespace
{
constexpr int kListenPort = 9527;
constexpr int kListenBacklog = 16;
constexpr std::size_t kBufferSize = 1024;
constexpr std::size_t kMaxEvents = 32;
constexpr std::size_t kMaxPendingOutputBytes = 64 * 1024;

struct ClientState
{
    std::string pending_output;
    bool peer_write_closed = false;
};

bool set_nonblocking(int file_descriptor)
{
    const int flags = ::fcntl(file_descriptor, F_GETFL, 0);
    if (flags == -1)
    {
        std::perror("fcntl(F_GETFL) failed");
        return false;
    }

    if (::fcntl(file_descriptor, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::perror("fcntl(F_SETFL) failed");
        return false;
    }

    return true;
}

bool send_all_blocking(int socket_fd, const char* data, std::size_t size)
{
    std::size_t sent_total = 0;
    while (sent_total < size)
    {
        const ssize_t sent = ::send(socket_fd,
            data + sent_total,
            size - sent_total,
            MSG_NOSIGNAL);
        if (sent > 0)
        {
            sent_total += static_cast<std::size_t>(sent);
            continue;
        }

        if (sent == -1 && errno == EINTR)
            continue;

        std::perror("send failed");
        return false;
    }

    return true;
}

bool receive_exactly(int socket_fd, std::size_t expected_size, std::string& received)
{
    char buffer[kBufferSize];
    while (received.size() < expected_size)
    {
        const std::size_t remaining_size = expected_size - received.size();
        const std::size_t wanted = remaining_size < sizeof(buffer)
            ? remaining_size
            : sizeof(buffer);
        const ssize_t count = ::recv(socket_fd, buffer, wanted, 0);
        if (count > 0)
        {
            received.append(buffer, static_cast<std::size_t>(count));
            continue;
        }

        if (count == 0)
        {
            std::fputs("server closed the connection before the complete echo arrived\n", stderr);
            return false;
        }

        if (errno == EINTR)
            continue;

        std::perror("recv failed");
        return false;
    }

    return true;
}

bool update_client_interest(int epoll_fd, int client_fd, const ClientState& state)
{
    epoll_event event {};
    event.events = EPOLLRDHUP;
    if (!state.peer_write_closed)
        event.events |= EPOLLIN;
    if (!state.pending_output.empty())
        event.events |= EPOLLOUT;
    event.data.fd = client_fd;

    if (::epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client_fd, &event) == -1)
    {
        std::perror("epoll_ctl(EPOLL_CTL_MOD) failed");
        return false;
    }

    return true;
}

void close_client(int epoll_fd,
    int client_fd,
    std::unordered_map<int, ClientState>& clients)
{
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr) == -1 && errno != ENOENT)
        std::perror("epoll_ctl(EPOLL_CTL_DEL) failed");

    ::close(client_fd);
    clients.erase(client_fd);
    std::cout << "[server] client fd=" << client_fd << " disconnected\n";
}

bool read_client(int client_fd, ClientState& state)
{
    char buffer[kBufferSize];
    while (true)
    {
        const ssize_t count = ::recv(client_fd, buffer, sizeof(buffer), 0);
        if (count > 0)
        {
            const std::size_t received_size = static_cast<std::size_t>(count);
            if (state.pending_output.size() + received_size > kMaxPendingOutputBytes)
            {
                std::fputs("[server] client output buffer limit reached\n", stderr);
                return false;
            }

            // This is an echo server, so received bytes are queued for sending back.
            state.pending_output.append(buffer, received_size);
            continue;
        }

        if (count == 0)
        {
            state.peer_write_closed = true;
            return true;
        }

        if (errno == EINTR)
            continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;

        std::perror("recv failed");
        return false;
    }
}

bool flush_client_output(int client_fd, ClientState& state)
{
    while (!state.pending_output.empty())
    {
        const ssize_t sent = ::send(client_fd,
            state.pending_output.data(),
            state.pending_output.size(),
            MSG_NOSIGNAL);
        if (sent > 0)
        {
            state.pending_output.erase(0, static_cast<std::size_t>(sent));
            continue;
        }

        if (sent == -1 && errno == EINTR)
            continue;
        if (sent == -1 && (errno == EAGAIN || errno == EWOULDBLOCK))
            return true;

        std::perror("send failed");
        return false;
    }

    return true;
}

bool accept_pending_clients(int epoll_fd,
    int server_fd,
    std::unordered_map<int, ClientState>& clients)
{
    while (true)
    {
        sockaddr_in client_address {};
        socklen_t client_address_length = sizeof(client_address);
        const int client_fd = ::accept(server_fd,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_length);
        if (client_fd == -1)
        {
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return true;

            std::perror("accept failed");
            return false;
        }

        if (!set_nonblocking(client_fd))
        {
            ::close(client_fd);
            continue;
        }

        epoll_event event {};
        event.events = EPOLLIN | EPOLLRDHUP;
        event.data.fd = client_fd;
        if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1)
        {
            std::perror("epoll_ctl(EPOLL_CTL_ADD client) failed");
            ::close(client_fd);
            continue;
        }

        clients.emplace(client_fd, ClientState {});
        char client_ip[INET_ADDRSTRLEN] {};
        const char* ip_text = ::inet_ntop(AF_INET,
            &client_address.sin_addr,
            client_ip,
            sizeof(client_ip));
        std::cout << "[server] client connected: "
                  << (ip_text == nullptr ? "unknown" : ip_text)
                  << ':' << ntohs(client_address.sin_port)
                  << ", fd=" << client_fd << '\n';
    }
}

void run_server()
{
    const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::perror("socket failed");
        return;
    }

    int reuse_address = 1;
    if (::setsockopt(server_fd,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse_address,
        sizeof(reuse_address)) == -1)
    {
        std::perror("setsockopt(SO_REUSEADDR) failed");
        ::close(server_fd);
        return;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(kListenPort);
    if (::bind(server_fd,
        reinterpret_cast<const sockaddr*>(&server_address),
        sizeof(server_address)) == -1)
    {
        std::perror("bind failed");
        ::close(server_fd);
        return;
    }

    if (::listen(server_fd, kListenBacklog) == -1)
    {
        std::perror("listen failed");
        ::close(server_fd);
        return;
    }

    if (!set_nonblocking(server_fd))
    {
        ::close(server_fd);
        return;
    }

    const int epoll_fd = ::epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1)
    {
        std::perror("epoll_create1 failed");
        ::close(server_fd);
        return;
    }

    epoll_event listen_event {};
    listen_event.events = EPOLLIN;
    listen_event.data.fd = server_fd;
    if (::epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &listen_event) == -1)
    {
        std::perror("epoll_ctl(EPOLL_CTL_ADD server) failed");
        ::close(epoll_fd);
        ::close(server_fd);
        return;
    }

    std::array<epoll_event, kMaxEvents> events {};
    std::unordered_map<int, ClientState> clients;
    std::cout << "[server] epoll echo server listening on 0.0.0.0:"
              << kListenPort << '\n';

    while (true)
    {
        const int event_count = ::epoll_wait(epoll_fd,
            events.data(),
            static_cast<int>(events.size()),
            -1);
        if (event_count == -1)
        {
            if (errno == EINTR)
                continue;

            std::perror("epoll_wait failed");
            break;
        }

        for (int index = 0; index < event_count; ++index)
        {
            const int ready_fd = events[static_cast<std::size_t>(index)].data.fd;
            const std::uint32_t ready_events = events[static_cast<std::size_t>(index)].events;

            if (ready_fd == server_fd)
            {
                if (!accept_pending_clients(epoll_fd, server_fd, clients))
                    std::fputs("[server] failed to accept a pending client\n", stderr);
                continue;
            }

            const auto client = clients.find(ready_fd);
            if (client == clients.end())
                continue;

            bool keep_client = true;
            if ((ready_events & EPOLLERR) != 0)
            {
                std::fputs("[server] client socket error\n", stderr);
                keep_client = false;
            }

            if (keep_client && (ready_events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP)) != 0)
                keep_client = read_client(ready_fd, client->second);

            if (keep_client && !client->second.pending_output.empty())
                keep_client = flush_client_output(ready_fd, client->second);

            if (keep_client && client->second.peer_write_closed &&
                client->second.pending_output.empty())
            {
                keep_client = false;
            }

            if (keep_client)
                keep_client = update_client_interest(epoll_fd, ready_fd, client->second);

            if (!keep_client)
                close_client(epoll_fd, ready_fd, clients);
        }
    }

    for (const auto& client : clients)
        ::close(client.first);
    ::close(epoll_fd);
    ::close(server_fd);
}

void run_client()
{
    const int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        std::perror("socket failed");
        return;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(kListenPort);
    if (::inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) != 1)
    {
        std::fputs("invalid server address\n", stderr);
        ::close(client_fd);
        return;
    }

    if (::connect(client_fd,
        reinterpret_cast<const sockaddr*>(&server_address),
        sizeof(server_address)) == -1)
    {
        std::perror("connect failed");
        ::close(client_fd);
        return;
    }

    std::puts("[client] connected. Enter a line; enter q to quit.");
    char message[kBufferSize];
    while (true)
    {
        std::cout << "> " << std::flush;
        if (std::fgets(message, sizeof(message), stdin) == nullptr)
            break;
        if (std::strcmp(message, "q\n") == 0 || std::strcmp(message, "Q\n") == 0)
            break;

        const std::size_t message_size = std::strlen(message);
        if (!send_all_blocking(client_fd, message, message_size))
            break;

        std::string echoed_message;
        if (!receive_exactly(client_fd, message_size, echoed_message))
            break;

        std::cout << "[client] echoed: " << echoed_message;
        if (echoed_message.empty() || echoed_message.back() != '\n')
            std::cout << '\n';
    }

    ::close(client_fd);
}
} // namespace

void IO_EPOLL_TEST(const char* mode)
{
    if (mode == nullptr)
    {
        std::fputs("mode is required: s for server or c for client\n", stderr);
        return;
    }

    if (std::strcmp(mode, "s") == 0)
    {
        run_server();
        return;
    }

    if (std::strcmp(mode, "c") == 0)
    {
        run_client();
        return;
    }

    std::fputs("invalid mode: use s for server or c for client\n", stderr);
}
