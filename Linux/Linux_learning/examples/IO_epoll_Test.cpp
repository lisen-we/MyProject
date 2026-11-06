#include "IO_epoll_Test.h"
#include <sys/epoll.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <wait.h>
#include <string>
#include <iostream>
namespace {
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
    void run_client()
    {
        const int client_socket = ::socket(AF_INET, SOCK_STREAM, 0);
        if (client_socket == -1)
        {
            std::perror("client socket failed");
            return;
        }

        sockaddr_in server_address{};
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
    void run_server()
    {
        int server_socket, client_socket;
        sockaddr_in server_addr,client_addr;
        socklen_t client_addr_len;
        server_socket = socket(PF_INET, SOCK_STREAM, 0);
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
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(9527);
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr))==-1);
        {
            std::perror("bind failed");
            ::close(server_socket);
            return;
        }
        if (::listen(server_socket, kListenBacklog) == -1)
        {
            std::perror("listen failed");
            ::close(server_socket);
            return;
        }
        char buffer[1024] = {};
        // 准备epoll事件
        epoll_event event;
        int epfd, event_cnt;
        epfd = epoll_create(1);
        if (epfd == -1)
        {
            std::perror("epoll failed");
            ::close(server_socket);
            return;
        }
        epoll_event* all_events = new epoll_event[100];
        event.events = EPOLLIN; // 事件设置为有数据输入
        event.data.fd = server_socket;
        epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &event);
        while (true)
        {
            event_cnt = epoll_wait(epfd, all_events, 100, 1000);//等待一秒种
            if (event_cnt == -1)
            {
                std::perror("epoll_wait failed");
                break;
            }
            if (event_cnt == 0)
            {
                continue;
            }
            for (int i = 0; i < event_cnt; i++)
            {
                //epoll和select的区别，select需要遍历所有的描述符，
                // 但是epoll只需要遍历count数量的事件
                if (all_events[i].data.fd == server_socket)
                {
                    //发生事件的是服务器，说明有client的connect
                    client_addr_len = sizeof(client_addr);
                    client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
                    event.events = EPOLLIN;
                    event.data.fd = client_socket;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &event);
                    std::cout << "client is connected" << std::endl;
                }
                else {
                    ssize_t len = read(all_events[i].data.fd, buffer, sizeof(buffer));
                    if (len <= 0)
                    {
                        epoll_ctl(epfd, EPOLL_CTL_DEL, all_events[i].data.fd, NULL);
                        close(all_events[i].data.fd);
                        std::cout << "client is closed" << std::endl;
                    }
                    else
                    {
                        write(all_events[i].data.fd, buffer, len);
                    }
                }
            }
        }


        delete[] all_events;
        close(epfd);
        close(server_socket);
    }
}
void IO_EPOLL_TEST(const char* mode)
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