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
// 本示例只监听本机回环地址，避免误将学习用服务器暴露到局域网。
constexpr std::uint16_t kPort = 10086;
// 服务端和客户端数量保持一致，因此程序可自行结束，方便观察完整调用过程。
constexpr std::size_t kDemoClientCount = 3;

// 每个进程都输出自己的 PID。日志顺序可能交错，这正是并发执行的表现。
void log_process(const char* role, const std::string& message)
{
    std::cout << '[' << role << ", pid=" << ::getpid() << "] "
              << message << '\n' << std::flush;
}

bool write_all(int fd, const char* data, std::size_t size)
{
    // TCP 是字节流：一次 send() 可能只写入部分数据，必须循环发送剩余部分。
    std::size_t total_written = 0;
    while (total_written < size)
    {
        // 对端提前断开时，普通 send 可能触发 SIGPIPE 并杀死当前进程。
        // MSG_NOSIGNAL 将其变成普通错误返回，交由代码处理。
        const ssize_t written = ::send(
            fd, data + total_written, size - total_written, MSG_NOSIGNAL);
        if (written > 0)
        {
            total_written += static_cast<std::size_t>(written);
            continue;
        }
        // 系统调用可能被信号中断，此时重试即可。
        if (written == -1 && errno == EINTR)
            continue;

        std::perror("send failed");
        return false;
    }
    return true;
}

bool read_exactly(int fd, std::size_t expected_size, std::string& received)
{
    // 客户端知道自己发送了多少字节，所以循环读取到同样长度，验证回声完整。
    // 不能假设一次 read() 就能读到一条完整的 TCP 消息。
    char buffer[256];
    while (received.size() < expected_size)
    {
        // 读取长度既不能超过局部缓冲区，也不必超过本条回声剩余长度。
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
    // 父进程需调用 wait/waitpid 回收自己创建的子进程，否则会产生僵尸进程。
    int status = 0;
    pid_t result = -1;
    do
    {
        result = ::waitpid(child_pid, &status, 0);
    } while (result == -1 && errno == EINTR); // 被信号打断时继续等待同一个子进程。

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
    // 此函数只在工作子进程中执行，因此慢客户端不会阻塞服务端父进程接收新连接。
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
        // read 返回 0 表示对端发送了 FIN，不会再向该连接写入数据。
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
    // SOCK_STREAM 表示 TCP，只有 TCP 才配合 bind/listen/accept 使用。
    const int server_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        std::perror("server socket failed");
        return EXIT_FAILURE;
    }

    // 反复运行演示时，旧连接可能处于 TIME_WAIT。SO_REUSEADDR 需在 bind 前设置，
    // 可降低端口暂未释放导致 bind 失败的概率。
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
    // 127.0.0.1：只接收本机自动创建的演示客户端。
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

    // 先 listen 成功，再通过 pipe 通知启动器创建客户端。否则客户端可能过早
    // connect，在服务端尚未就绪时得到 connection refused。
    const char ready = 'R';
    if (::write(ready_pipe_write_fd, &ready, sizeof(ready)) != sizeof(ready))
    {
        std::perror("notify parent failed");
        ::close(server_fd);
        return EXIT_FAILURE;
    }
    log_process("server", "listening on 127.0.0.1:" + std::to_string(kPort));

    // 保存 worker PID，服务端父进程会在最后逐个 waitpid 回收它们。
    std::array<pid_t, kDemoClientCount> worker_pids {};
    for (std::size_t index = 0; index < kDemoClientCount; ++index)
    {
        sockaddr_in client_address {};
        socklen_t client_address_length = sizeof(client_address);

        // accept 创建的是“已连接 socket”；监听 socket 仍用于接收后续连接。
        int client_fd = -1;
        do
        {
            client_fd = ::accept(server_fd,
                (struct sockaddr*)(&client_address), &client_address_length);
        } while (client_fd == -1 && errno == EINTR);

        if (client_fd == -1)
        {
            std::perror("accept failed");
            ::close(server_fd);
            return EXIT_FAILURE;
        }

        log_process("server", "accepted client #" + std::to_string(index + 1));
        // 并发的核心：每次 accept 后 fork 一个 worker。父进程立刻回到 accept，
        // 子进程独立处理当前连接，因此多个客户端可同时收发数据。
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
            // fork 会复制所有文件描述符，但 worker 只需要 client_fd。关闭复制的
            // server_fd 可避免它意外 accept，也避免监听 socket 被多余地持有。
            ::close(server_fd);
            const int result = echo_one_client(client_fd);
            ::close(client_fd);
            // 子进程完成后不能继续执行启动器代码；_exit 可直接结束该子进程。
            _exit(result);
        }

        worker_pids[index] = worker_pid;
        // 父进程不处理当前连接，关闭自己复制的 client_fd。否则 worker 即使关闭，
        // 内核仍会认为父进程持有该连接，可能影响连接正常结束。
        ::close(client_fd);
        log_process("server", "forked worker pid=" + std::to_string(worker_pid) +
            "; returning immediately to accept()");
    }

    // 本演示的 worker 数量固定，逐个 waitpid 最容易理解。真实的长期服务器
    // 通常注册 SIGCHLD，并在处理函数中使用 waitpid(-1, ..., WNOHANG)
    // 非阻塞地回收所有已结束的 worker。
    for (const pid_t worker_pid : worker_pids)
        wait_for_child(worker_pid, "server");

    ::close(server_fd);
    log_process("server", "all worker processes were reaped; server exits");
    return EXIT_SUCCESS;
}

int run_demo_client(int client_number)
{
    // 这些客户端由启动器自动创建，只用于本机演示；真实客户端通常来自其他电脑。
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
    // 每发送一条就等待对应回声，使客户端能明确知道本次应读回多少字节。
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

    // shutdown(SHUT_WR) 只关闭“写方向”，向服务端发送 FIN。服务端 read 返回 0
    // 后结束 worker；这里用于展示 TCP 半关闭，而非直接丢弃连接。
    if (::shutdown(client_fd, SHUT_WR) == -1)
        // shutdown的作用是向服务器发送FIN信号，此时客户端进入半关闭，可以继续read，但是无法write
        std::perror("client shutdown failed");
    ::close(client_fd);
    log_process("client", "client #" + std::to_string(client_number) + " exits");
    return EXIT_SUCCESS;
}
} // namespace

void ConcurrentServer()
{
    // pipe 只传递一个“服务端已 listen”的字节，用于消除客户端启动过早的竞争。
    int ready_pipe[2];
    if (::pipe(ready_pipe) == -1)
    {
        std::perror("pipe failed");
        return;
    }
    // pipe成功之后ready_pipe中0，1位置保存的是文件标识符，0位置为读取的文件标识符
    // 1位置为写入的文件标识符
    // 第一次 fork 分离两种角色：子进程运行服务端，父进程作为启动器创建客户端。
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
        // 经过fork之后，父进程和子进程各自拥有一份文件标识符
        // 子进程尽早关闭自己的不使用的读取端文件标识符，确保管道的 EOF、SIGPIPE 等语义正确
        // 服务端只写就绪通知，不读取 pipe。
        ::close(ready_pipe[0]);
        const int result = run_server(ready_pipe[1]);
        ::close(ready_pipe[1]);
        //使用_exit立即退出，不清理缓冲区
        _exit(result);
    }

    // 启动器只读取就绪通知，不写 pipe。
    ::close(ready_pipe[1]);
    char server_ready = '\0';
    const ssize_t readiness_count = ::read(ready_pipe[0], &server_ready,sizeof(server_ready));
    ::close(ready_pipe[0]);
    // 只要父进程读取到了pipe中的内容，并且这个内容的值为R则说明之前的子进程已经成功开启了服务器。
    if (readiness_count != 1 || server_ready != 'R')
    {
        std::cerr << "server did not become ready; no client will be started\n";
        wait_for_child(server_pid, "launcher");
        return;
    }

    log_process("launcher", "server is ready; creating 3 demo clients");
    // 第二组 fork 创建三个独立客户端。它们由内核调度，可与多个 worker 交错执行；
    // 在多核 CPU 上，部分操作还可能真正并行。
    std::array<pid_t, kDemoClientCount> client_pids {};
    // 创建三个子进程，每个子进程各自持有一个客户端来连接服务器
    for (std::size_t index = 0; index < kDemoClientCount; ++index)
    {
        const pid_t client_pid = ::fork();
        if (client_pid == -1)
        {
            std::perror("fork demo client failed");
            continue;
        }
        if (client_pid == 0)
            // 客户端完成后不能落入下面的父进程回收循环。
            _exit(run_demo_client(static_cast<int>(index + 1)));

        client_pids[index] = client_pid;
    }

    // 启动器回收三个客户端和一个服务端；服务端则回收自己的 worker。
    // 因此每个子进程都有仍存活的父进程负责回收，不会产生僵尸进程。
    for (const pid_t client_pid : client_pids)
    {
        if (client_pid > 0)
            wait_for_child(client_pid, "launcher");
    }
    wait_for_child(server_pid, "launcher");
    log_process("launcher", "concurrent echo-server demonstration finished");
}
