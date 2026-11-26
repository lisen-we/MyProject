#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <cerrno>
#include <unistd.h>
#include <cstring>
#include <string>
#include <array>
#include <unordered_map>
#include <stdexcept>
#include <iostream>
#include "ClientMessage.h"
#include "ServerMessage.h"
namespace
{
    constexpr int maxClientCount = 100;
    constexpr int serverport = 9527;
    // 注册响应沿用 8 字节消息头，正文 1 字节：1 成功，0 失败。

    void run_server()
    {
        int server_socket, client_socket;
        sockaddr_in server_addr{}, client_addr{};
        socklen_t client_addr_len;
        int permissionEnable = 1;

        server_socket = socket(PF_INET, SOCK_STREAM, 0);
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &permissionEnable, sizeof(permissionEnable));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        server_addr.sin_port = htons(serverport);
        bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
        listen(server_socket, maxClientCount);
        //epoll逻辑
        epoll_event listen_event{};
        const int epfd = epoll_create1(EPOLL_CLOEXEC);

        listen_event.data.fd = server_socket;
        listen_event.events = EPOLLIN; //默认使用水平触发
        //把 server_socket 加入 epfd 的监听集合，监听规则由 listen_event.events 指定
        epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &listen_event);
        //events 用于接收本次哪些对象就绪了
        std::array<epoll_event, 100> events{};
        while (true)
        {
            const int count = epoll_wait(epfd, events.data(), static_cast<int>(events.size()), -1);
            if (count == -1)
            {
                if (errno == EINTR)
                    continue;
                break;
            }
            else if (count > 0)
            {
                for (int i = 0; i < count; i++)
                {
                    const int ready_fd = events[i].data.fd;   //接收到的发生事件的文件描述符
                    const int ready_fd_event = events[i].events;  //接收到的实际事件
                    if (ready_fd == server_socket)
                    {
                        client_addr_len = sizeof(client_addr);
                        client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
                        epoll_event client_event{};
                        client_event.data.fd = client_socket;
                        client_event.events = EPOLLIN;
                        epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &client_event);
                        sessions.emplace(client_socket, clientSession{});
                    }
                    else if ((ready_fd_event & EPOLLIN) != 0)
                    {
                        char message[1024] = {};
                        const ssize_t receive_count = read(ready_fd, message, sizeof(message));
                        if (receive_count <= 0)
                        {
                            //读取失败需要将client从epfd实例中删掉
                            epoll_ctl(epfd, EPOLL_CTL_DEL, ready_fd, nullptr);
                            close(ready_fd); 
                            sessions.erase(ready_fd);
                            continue;
                        }
                        bool result = AppendClientMessage(ready_fd,message, receive_count);
                        if (result)
                        {
                            //数据接收完全从缓冲区中取出一帧进行解析
                            try {
                                while (ProcessClientMessage(ready_fd))
                                {
                                    const auto& parsed = sessions.at(ready_fd).parsedMessage;
                                    switch (parsed.header.type)
                                    {
                                    
                                    case ClientCommandType::CreateUserRequest:
                                    {
                                        const bool registered = HandleCreateUser(parsed);
                                        std::cout << "Client fd = " << ready_fd
                                                  << (registered ? " registration succeeded" : " registration failed")
                                                  << std::endl;
                                        // 发送失败时关闭连接，避免客户端收到不完整帧后继续解析。
                                        if (!SendCreateUserResponse(ready_fd, registered))
                                            throw std::runtime_error("Failed to send registration response");
                                        break;
                                    }
                                    case ClientCommandType::LoginRequest:
                                    {
                                        const bool loggedIn = HandleLoginRequest(parsed);
                                        clientSession& session = sessions.at(ready_fd);
                                        session.loggedIn = loggedIn;
                                        session.account = loggedIn ? parsed.login.account : std::string();
                                        if (!SendUserLoginResponse(ready_fd, loggedIn))
                                            throw std::runtime_error("Failed to send login response");
                                        break;
                                    }
                                    case ClientCommandType::GroupChatRequest:
                                    {
                                        break;
                                    }
                                    case ClientCommandType::FileChunk:
                                    {
                                        break;
                                    }
                                    }
                                }
                                continue;
                            }
                            catch (const std::runtime_error& error)
                            {
                                std::cout << "Client fd = " << ready_fd << " error: " << error.what() << std::endl << std::flush;
                                epoll_ctl(epfd, EPOLL_CTL_DEL, ready_fd,nullptr);
                                close(ready_fd);
                                sessions.erase(ready_fd);
                                continue;
                            }
                        }
                    }
                }
            }
        }
    }
}//namespace
int main()
{
    run_server();
    return 0;
}