#include "mutiThread_EchoServer.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <new>
#include <pthread.h>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

namespace
{
constexpr int kListenPort = 10000;
constexpr int kMaxClients = 100;
constexpr std::size_t kReadBufferSize = 1024;
constexpr std::size_t kMaxPendingMessageSize = 4096;

int client_sockets[kMaxClients];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

bool send_all(int client_socket, const char* data, std::size_t length)
{
    std::size_t total_sent = 0;
    while (total_sent < length)
    {
        // MSG_NOSIGNAL 防止向已断开的客户端广播时触发 SIGPIPE，进而终止
        // 整个服务端进程。
        const ssize_t sent = ::send(client_socket,
            data + total_sent,
            length - total_sent,
            MSG_NOSIGNAL);
        if (sent > 0)
        {
            total_sent += static_cast<std::size_t>(sent);
            continue;
        }

        if (sent == -1 && errno == EINTR)
            continue;

        return false;
    }

    return true;
}

int add_client(int client_socket)
{
    int available_slot = -1;
    ::pthread_mutex_lock(&clients_mutex);
    for (int index = 0; index < kMaxClients; ++index)
    {
        if (client_sockets[index] == -1)
        {
            client_sockets[index] = client_socket;
            available_slot = index;
            break;
        }
    }
    ::pthread_mutex_unlock(&clients_mutex);
    return available_slot;
}

void remove_client(int client_socket)
{
    ::pthread_mutex_lock(&clients_mutex);
    for (int index = 0; index < kMaxClients; ++index)
    {
        if (client_sockets[index] == client_socket)
        {
            client_sockets[index] = -1;
            break;
        }
    }
    ::pthread_mutex_unlock(&clients_mutex);
}

void broadcast_message(const char* message, std::size_t message_length)
{
    // 此入门实现会在发送期间保持客户端列表锁，防止一个线程关闭描述符时，
    // 另一个线程仍在向它广播。生产环境可使用每客户端发送队列或非阻塞 I/O，
    // 避免一个慢客户端拖慢全部广播。
    ::pthread_mutex_lock(&clients_mutex);
    for (int index = 0; index < kMaxClients; ++index)
    {
        const int client_socket = client_sockets[index];
        if (client_socket == -1)
            continue;

        if (!send_all(client_socket, message, message_length))
        {
            std::fprintf(stderr, "[server] send to client fd=%d failed: %s\n",
                client_socket,
                std::strerror(errno));

            // 此处不能 close：拥有该描述符的工作线程可能正阻塞在 read()。
            // shutdown 会唤醒该线程，由它移除客户端并执行最终的 close。
            client_sockets[index] = -1;
            ::shutdown(client_socket, SHUT_RDWR);
        }
    }
    ::pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* argument)
{
    // accept 线程为当前工作线程分配了稳定的参数。先复制描述符，再释放参数，
    // 然后进入持续接收循环。
    auto* client_socket_argument = static_cast<int*>(argument);
    const int client_socket = *client_socket_argument;
    delete client_socket_argument;

    std::string pending_message;
    char read_buffer[kReadBufferSize];
    bool connected = true;

    while (connected)
    {
        const ssize_t received = ::read(client_socket, read_buffer, sizeof(read_buffer));
        if (received > 0)
        {
            pending_message.append(read_buffer, static_cast<std::size_t>(received));

            while (connected)
            {
                const std::size_t newline_position = pending_message.find('\n');
                if (newline_position == std::string::npos)
                    break;

                std::string one_message = pending_message.substr(0, newline_position + 1);
                pending_message.erase(0, newline_position + 1);

                // Netcat 通常发送 "\r\n"。检查命令时仅移除行结束符；广播时
                // 保留原始行，确保所有客户端仍能收到换行结尾的文本。
                std::string command = one_message;
                while (!command.empty() &&
                    (command.back() == '\n' || command.back() == '\r'))
                {
                    command.pop_back();
                }

                if (command == "quit")
                {
                    connected = false;
                    break;
                }

                broadcast_message(one_message.data(), one_message.size());
            }

            if (pending_message.size() > kMaxPendingMessageSize)
            {
                constexpr char kTooLongMessage[] =
                    "message exceeds the 4096-byte line limit; closing connection\n";
                send_all(client_socket, kTooLongMessage, sizeof(kTooLongMessage) - 1);
                connected = false;
            }
            continue;
        }

        if (received == 0)
        {
            // 对端进行了正常的 TCP 连接关闭。
            break;
        }

        if (errno == EINTR)
            continue;

        std::fprintf(stderr, "[server] read from client fd=%d failed: %s\n",
            client_socket,
            std::strerror(errno));
        break;
    }

    remove_client(client_socket);
    ::close(client_socket);
    std::printf("[server] client fd=%d disconnected\n", client_socket);
    return nullptr;
}

void run_server()
{
    for (int index = 0; index < kMaxClients; ++index)
        client_sockets[index] = -1;

    const int server_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::perror("socket failed");
        return;
    }

    int reuse_address = 1;
    if (::setsockopt(server_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse_address,
        sizeof(reuse_address)) == -1)
    {
        std::perror("setsockopt SO_REUSEADDR failed");
        ::close(server_socket);
        return;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(kListenPort);
    if (::bind(server_socket,
        reinterpret_cast<const sockaddr*>(&server_address),
        sizeof(server_address)) == -1)
    {
        std::perror("bind failed");
        ::close(server_socket);
        return;
    }
    //最多监听100个客户端
    if (::listen(server_socket, kMaxClients) == -1)
    {
        std::perror("listen failed");
        ::close(server_socket);
        return;
    }

    pthread_attr_t detached_attributes;
    int pthread_result = ::pthread_attr_init(&detached_attributes);
    if (pthread_result != 0)
    {
        std::fprintf(stderr, "pthread_attr_init failed: %s\n", std::strerror(pthread_result));
        ::close(server_socket);
        return;
    }
    pthread_result = ::pthread_attr_setdetachstate(&detached_attributes, PTHREAD_CREATE_DETACHED);
    if (pthread_result != 0)
    {
        std::fprintf(stderr, "pthread_attr_setdetachstate failed: %s\n",
            std::strerror(pthread_result));
        ::pthread_attr_destroy(&detached_attributes);
        ::close(server_socket);
        return;
    }

    std::printf("[server] continuous chat server listening on 0.0.0.0:%d\n", kListenPort);
    std::puts("[server] protocol: send newline-terminated text; send quit to disconnect");

    while (true)
    {
        sockaddr_in client_address {};
        socklen_t client_address_length = sizeof(client_address);
        const int client_socket = ::accept(server_socket,
            reinterpret_cast<sockaddr*>(&client_address),
            &client_address_length);
        if (client_socket == -1)
        {
            if (errno == EINTR)
                continue;

            std::perror("accept failed");
            break;
        }

        const int client_slot = add_client(client_socket);
        if (client_slot == -1)
        {
            constexpr char kServerFullMessage[] = "server is full\n";
            send_all(client_socket, kServerFullMessage, sizeof(kServerFullMessage) - 1);
            ::close(client_socket);
            continue;
        }

        auto* client_socket_argument = new (std::nothrow) int(client_socket);
        if (client_socket_argument == nullptr)
        {
            std::fputs("cannot allocate client thread argument\n", stderr);
            remove_client(client_socket);
            ::close(client_socket);
            continue;
        }

        pthread_t thread;
        pthread_result = ::pthread_create(&thread,
            &detached_attributes,
            handle_client,
            client_socket_argument);
        if (pthread_result != 0)
        {
            std::fprintf(stderr, "pthread_create failed: %s\n", std::strerror(pthread_result));
            delete client_socket_argument;
            remove_client(client_socket);
            ::close(client_socket);
            continue;
        }

        char client_ip[INET_ADDRSTRLEN] {};
        const char* ip_text = ::inet_ntop(AF_INET,
            &client_address.sin_addr,
            client_ip,
            sizeof(client_ip));
        std::printf("[server] client connected: %s:%u, fd=%d, slot=%d\n",
            ip_text == nullptr ? "unknown" : ip_text,
            static_cast<unsigned int>(ntohs(client_address.sin_port)),
            client_socket,
            client_slot);
    }

    ::pthread_attr_destroy(&detached_attributes);
    ::close(server_socket);
    // 工作线程已分离，仍可能使用 clients_mutex。该服务端设计为一直运行到进程
    // 整体退出，因此此错误路径不能销毁互斥量；优雅退出需要先实现工作线程计数。
}
}

void mutiThread_EchoServer()
{
    run_server();
}
