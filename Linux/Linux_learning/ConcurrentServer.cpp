#include "ConcurrentServer.h"

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
constexpr std::uint16_t kPort = 10086;
constexpr std::size_t kDemoClientCount = 3;

void log_process(const char* role, const std::string& message)
{
    std::cout << '[' << role << ", pid=" << ::getpid() << "] "
              << message << '\n' << std::flush;
}

bool write_all(int fd, const char* data, std::size_t size)
{
    std::size_t total_written = 0;
    while (total_written < size)
    {
        // MSG_NOSIGNAL prevents a disconnected peer from terminating this
        // process with SIGPIPE.
        const ssize_t written = ::send(
            fd, data + total_written, size - total_written, MSG_NOSIGNAL);
        if (written > 0)
        {
            total_written += static_cast<std::size_t>(written);
            continue;
        }
        if (written == -1 && errno == EINTR)
            continue;

        std::perror("send failed");
        return false;
    }
    return true;
}

bool read_exactly(int fd, std::size_t expected_size, std::string& received)
{
    char buffer[256];
    while (received.size() < expected_size)
    {
        const std::size_t wanted =
            std::min(sizeof(buffer), expected_size - received.size());
        const ssize_t count = ::read(fd, buffer, wanted);
        if (count > 0)
        {
            received.append(buffer, static_cast<std::size_t>(count));
            continue;
        }
        if (count == 0)
        {
            std::cerr << "server closed the connection too early\n";
            return false;
        }
        if (errno == EINTR)
            continue;

        std::perror("client read failed");
        return false;
    }
    return true;
}

void wait_for_child(pid_t child_pid, const char* role)
{
    int status = 0;
    pid_t result = -1;
    do
    {
        result = ::waitpid(child_pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1)
    {
        std::perror("waitpid failed");
        return;
    }

    if (WIFEXITED(status))
    {
        log_process(role, "reaped child pid=" + std::to_string(result) +
            ", exit code=" + std::to_string(WEXITSTATUS(status)));
    }
    else if (WIFSIGNALED(status))
    {
        log_process(role, "reaped child pid=" + std::to_string(result) +
            ", terminating signal=" + std::to_string(WTERMSIG(status)));
    }
}

int echo_one_client(int client_fd)
{
    char buffer[2048];
    while (true)
    {
        const ssize_t count = ::read(client_fd, buffer, sizeof(buffer));
        if (count > 0)
        {
            log_process("worker", "read " + std::to_string(count) +
                " bytes; echoing them back");
            if (!write_all(client_fd, buffer, static_cast<std::size_t>(count)))
                return EXIT_FAILURE;
            continue;
        }
        if (count == 0)
        {
            log_process("worker", "client closed its write side");
            return EXIT_SUCCESS;
        }
        if (errno == EINTR)
            continue;

        std::perror("worker read failed");
        return EXIT_FAILURE;
    }
}

int run_server(int ready_pipe_write_fd)
{
    const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::perror("server socket failed");
        return EXIT_FAILURE;
    }

    int reuse_address = 1;
    if (::setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
        &reuse_address, sizeof(reuse_address)) == -1)
    {
        std::perror("setsockopt SO_REUSEADDR failed");
        ::close(server_fd);
        return EXIT_FAILURE;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(kPort);

    if (::bind(server_fd, reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)) == -1)
    {
        std::perror("bind failed");
        ::close(server_fd);
        return EXIT_FAILURE;
    }

    if (::listen(server_fd, static_cast<int>(kDemoClientCount)) == -1)
    {
        std::perror("listen failed");
        ::close(server_fd);
        return EXIT_FAILURE;
    }

    const char ready = 'R';
    if (::write(ready_pipe_write_fd, &ready, sizeof(ready)) != sizeof(ready))
    {
        std::perror("notify parent failed");
        ::close(server_fd);
        return EXIT_FAILURE;
    }
    log_process("server", "listening on 127.0.0.1:" + std::to_string(kPort));

    std::array<pid_t, kDemoClientCount> worker_pids {};
    for (std::size_t index = 0; index < kDemoClientCount; ++index)
    {
        sockaddr_in client_address {};
        socklen_t client_address_length = sizeof(client_address);

        int client_fd = -1;
        do
        {
            client_fd = ::accept(server_fd,
                reinterpret_cast<sockaddr*>(&client_address), &client_address_length);
        } while (client_fd == -1 && errno == EINTR);

        if (client_fd == -1)
        {
            std::perror("accept failed");
            ::close(server_fd);
            return EXIT_FAILURE;
        }

        log_process("server", "accepted client #" + std::to_string(index + 1));
        const pid_t worker_pid = ::fork();
        if (worker_pid == -1)
        {
            std::perror("fork worker failed");
            ::close(client_fd);
            ::close(server_fd);
            return EXIT_FAILURE;
        }

        if (worker_pid == 0)
        {
            // The worker needs this client's socket only.  Closing the copied
            // listening socket is important for correct descriptor ownership.
            ::close(server_fd);
            const int result = echo_one_client(client_fd);
            ::close(client_fd);
            _exit(result);
        }

        worker_pids[index] = worker_pid;
        ::close(client_fd);
        log_process("server", "forked worker pid=" + std::to_string(worker_pid) +
            "; returning immediately to accept()");
    }

    // This finite demo has a known number of workers, so direct waitpid()
    // makes their cleanup explicit.  A never-ending server commonly uses a
    // SIGCHLD handler with waitpid(-1, ..., WNOHANG) instead.
    for (const pid_t worker_pid : worker_pids)
        wait_for_child(worker_pid, "server");

    ::close(server_fd);
    log_process("server", "all worker processes were reaped; server exits");
    return EXIT_SUCCESS;
}

int run_demo_client(int client_number)
{
    const int client_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1)
    {
        std::perror("client socket failed");
        return EXIT_FAILURE;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    server_address.sin_port = htons(kPort);

    if (::connect(client_fd, reinterpret_cast<sockaddr*>(&server_address),
        sizeof(server_address)) == -1)
    {
        std::perror("connect failed");
        ::close(client_fd);
        return EXIT_FAILURE;
    }

    log_process("client", "client #" + std::to_string(client_number) +
        " connected to the server");
    const std::array<std::string, 2> messages = {
        "client " + std::to_string(client_number) + ": first message\n",
        "client " + std::to_string(client_number) + ": second message\n"
    };

    for (const std::string& message : messages)
    {
        if (!write_all(client_fd, message.data(), message.size()))
        {
            ::close(client_fd);
            return EXIT_FAILURE;
        }

        std::string echoed_message;
        if (!read_exactly(client_fd, message.size(), echoed_message))
        {
            ::close(client_fd);
            return EXIT_FAILURE;
        }

        log_process("client", "received echo: " + echoed_message.substr(
            0, echoed_message.size() - 1));
    }

    if (::shutdown(client_fd, SHUT_WR) == -1)
        std::perror("client shutdown failed");
    ::close(client_fd);
    log_process("client", "client #" + std::to_string(client_number) + " exits");
    return EXIT_SUCCESS;
}
} // namespace

void ConcurrentServer()
{
    int ready_pipe[2];
    if (::pipe(ready_pipe) == -1)
    {
        std::perror("pipe failed");
        return;
    }

    const pid_t server_pid = ::fork();
    if (server_pid == -1)
    {
        std::perror("fork server failed");
        ::close(ready_pipe[0]);
        ::close(ready_pipe[1]);
        return;
    }

    if (server_pid == 0)
    {
        ::close(ready_pipe[0]);
        const int result = run_server(ready_pipe[1]);
        ::close(ready_pipe[1]);
        _exit(result);
    }

    ::close(ready_pipe[1]);
    char server_ready = '\0';
    const ssize_t readiness_count = ::read(ready_pipe[0], &server_ready,
        sizeof(server_ready));
    ::close(ready_pipe[0]);
    if (readiness_count != 1 || server_ready != 'R')
    {
        std::cerr << "server did not become ready; no client will be started\n";
        wait_for_child(server_pid, "launcher");
        return;
    }

    log_process("launcher", "server is ready; creating 3 demo clients");
    std::array<pid_t, kDemoClientCount> client_pids {};
    for (std::size_t index = 0; index < kDemoClientCount; ++index)
    {
        const pid_t client_pid = ::fork();
        if (client_pid == -1)
        {
            std::perror("fork demo client failed");
            continue;
        }
        if (client_pid == 0)
            _exit(run_demo_client(static_cast<int>(index + 1)));

        client_pids[index] = client_pid;
    }

    for (const pid_t client_pid : client_pids)
    {
        if (client_pid > 0)
            wait_for_child(client_pid, "launcher");
    }
    wait_for_child(server_pid, "launcher");
    log_process("launcher", "concurrent echo-server demonstration finished");
}
