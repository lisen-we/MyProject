#include "IO_Select_Test.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace
{
constexpr int kListenPort = 9527;
constexpr int kListenBacklog = 5;
constexpr std::size_t kBufferSize = 256;

bool send_all(int socket_fd, const char* data, std::size_t length)
{
    std::size_t total_sent = 0;
    while (total_sent < length)
    {
        // send 不保证一次写完全部字节；MSG_NOSIGNAL 防止客户端断开时触发
        // SIGPIPE 而终止整个服务端进程。
        const ssize_t sent = ::send(socket_fd,
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

void remove_client(fd_set* master_reads, int client_fd, int* max_fd)
{
    FD_CLR(client_fd, master_reads);
    ::close(client_fd);

    // select 的第一个参数是“最大文件描述符 + 1”。关闭当前最大描述符后，
    // 向下寻找仍在主集合中的最大值，避免每轮扫描无效描述符。
    while (*max_fd > 0 && !FD_ISSET(*max_fd, master_reads))
        --(*max_fd);
}

void run_server()
{
    // 创建套接字
    const int server_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::perror("socket failed");
        return;
    }
    // 在服务器关闭的时候，会短暂的处于time_wait状态，从而导致无法快速重启服务
    // 使用setsocketopt设置套接字的权限，给服务器套接字设置SOL_SOCKET下的SO_REUSEADDR权限
    // 让其更快地复用地址端口,reuse_address传入1表示开启该权限
    // setsocketopt的参数分别为：套接字，层级，具体选项，值的地址，长度
    // SOL = socket option level SO = socket option
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
    // 绑定服务器地址
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
    // listen 等待区最多只能容纳五个连接
    if (::listen(server_socket, kListenBacklog) == -1)
    {
        std::perror("listen failed");
        ::close(server_socket);
        return;
    }

    // master_reads 保存“长期监控”的描述符集合：监听 socket 和所有客户端
    // socket 都在这里。它不能直接传给 select，因为 select 会修改集合。
    fd_set master_reads;
    FD_ZERO(&master_reads);
    FD_SET(server_socket, &master_reads);
    int max_fd = server_socket;

    std::printf("[server] select echo server listening on 0.0.0.0:%d\n", kListenPort);
    while (true)
    {
        // 每次循环都会复制一次监听集合
        fd_set read_set = master_reads;

        // Linux 会修改 timeval 为剩余等待时间。每轮重新设置为 0.5 秒，
        // 才能让每一次 select 都拥有完整的超时等待时间。
        timeval timeout {};
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;

        // select() 会修改 read_set，只保留本轮发生可读事件的描述符。
        const int ready_count = ::select(max_fd + 1,
            &read_set,
            nullptr,
            nullptr,
            &timeout);
        if (ready_count == -1)
        {
            if (errno == EINTR)
                continue;

            std::perror("select failed");
            break;
        }
        if (ready_count == 0)
        {
            // 当前 0.5 秒内没有任何 socket 可读；继续下一轮等待。
            continue;
        }
        // read_set 是 select 修改后的“本轮可读就绪集合”。
        // 若监听 socket 可读，说明有新客户端连接等待 accept()。
        if (FD_ISSET(server_socket, &read_set))
        {
            sockaddr_in client_address {};
            socklen_t client_address_length = sizeof(client_address);
            const int client_socket = ::accept(server_socket,
                reinterpret_cast<sockaddr*>(&client_address),
                &client_address_length);
            if (client_socket == -1)
            {
                if (errno != EINTR)
                    std::perror("accept failed");
            }
            else if (client_socket >= FD_SETSIZE)
            {
                // fd_set中存放的是文件描述符，其范围为0~1023
                // fd_set 只能表示 [0, FD_SETSIZE - 1]。超出范围时不能 FD_SET，
                // 否则会越界写入 fd_set。
                //进程实际能打开的文件描述符数量可能超过 1024，但传统 select 的 fd_set 
                //默认无法安全监听编号大于等于 FD_SETSIZE 的描述符。
                //这也是高并发 Linux 服务端更常使用 poll 或 epoll 的原因之一
                std::fputs("client descriptor exceeds FD_SETSIZE; connection rejected\n", stderr);
                ::close(client_socket);
            }
            else
            {
                FD_SET(client_socket, &master_reads);
                if (client_socket > max_fd)
                    max_fd = client_socket;

                char client_ip[INET_ADDRSTRLEN] {};
                const char* ip_text = ::inet_ntop(AF_INET,
                    &client_address.sin_addr,
                    client_ip,
                    sizeof(client_ip));
                std::printf("[server] client connected: %s:%u, fd=%d\n",
                    ip_text == nullptr ? "unknown" : ip_text,
                    static_cast<unsigned int>(ntohs(client_address.sin_port)),
                    client_socket);
            }
        }

        // 遍历所有可能的文件描述符，找出本轮 select() 判断为“客户端可读”的 socket，并读取和处理它们的数据。
        for (int client_fd = 0; client_fd <= max_fd; ++client_fd)
        {
            // FD_ISSET用于判断发生可读事件的是否是当前传入的文件描述符
            if (client_fd == server_socket || !FD_ISSET(client_fd, &read_set))
                continue;

            char buffer[kBufferSize];
            const ssize_t received = ::read(client_fd, buffer, sizeof(buffer));
            if (received > 0)
            {
                // 本示例演示 select 的可读事件和回声。TCP 是字节流，实际协议
                // 可能出现拆包、粘包；若需要按行或按消息处理，应额外维护缓冲区。
                if (!send_all(client_fd, buffer, static_cast<std::size_t>(received)))
                {
                    std::fprintf(stderr, "[server] echo to fd=%d failed: %s\n",
                        client_fd,
                        std::strerror(errno));
                    remove_client(&master_reads, client_fd, &max_fd);
                }
            }
            else if (received == 0)
            {
                // read 返回 0 代表客户端已正常关闭写方向或整个连接。
                std::printf("[server] client fd=%d disconnected\n", client_fd);
                remove_client(&master_reads, client_fd, &max_fd);
            }
            else if (errno != EINTR)
            {
                std::fprintf(stderr, "[server] read from fd=%d failed: %s\n",
                    client_fd,
                    std::strerror(errno));
                remove_client(&master_reads, client_fd, &max_fd);
            }
        }
    }

    for (int client_fd = 0; client_fd <= max_fd; ++client_fd)
    {
        if (client_fd != server_socket && FD_ISSET(client_fd, &master_reads))
            ::close(client_fd);
    }
    ::close(server_socket);
}

void run_client()
{
    const int client_socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1)
    {
        std::perror("client socket failed");
        return;
    }

    sockaddr_in server_address {};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(kListenPort);
    if (::inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) != 1)
    {
        std::fputs("invalid server address\n", stderr);
        ::close(client_socket);
        return;
    }

    if (::connect(client_socket,
        reinterpret_cast<const sockaddr*>(&server_address),
        sizeof(server_address)) == -1)
    {
        std::perror("connect failed");
        ::close(client_socket);
        return;
    }

    std::puts("[client] connected. Input a line; input q to quit.");
    char message[kBufferSize];
    while (true)
    {
        std::cout << "input message (q to quit): ";
        if (std::fgets(message, sizeof(message), stdin) == nullptr)
            break;

        if (std::strcmp(message, "q\n") == 0 || std::strcmp(message, "Q\n") == 0)
            break;

        const std::size_t message_length = std::strlen(message);
        if (!send_all(client_socket, message, message_length))
        {
            std::perror("client send failed");
            break;
        }

        char echoed_message[kBufferSize];
        ssize_t received = -1;
        while (true)
        {
            received = ::read(client_socket, echoed_message, sizeof(echoed_message));
            if (received == -1 && errno == EINTR)
                continue;

            break;
        }
        if (received > 0)
        {
            std::cout << "server echoed: ";
            std::cout.write(echoed_message, received);
            if (echoed_message[received - 1] != '\n')
                std::cout << '\n';
        }
        else if (received == 0)
        {
            std::puts("[client] server closed the connection");
            break;
        }
        else
        {
            std::perror("client read failed");
            break;
        }
    }

    ::close(client_socket);
}
}

void IOSelectTest(const char* mode)
{
    if (mode == nullptr)
    {
        std::fputs("select mode is required: s for server or c for client\n", stderr);
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

    std::fputs("invalid select mode: use s for server or c for client\n", stderr);
}
