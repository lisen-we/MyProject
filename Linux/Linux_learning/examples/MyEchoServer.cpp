#include "MyEchoServer.h"
#include <iostream>
#include <cstring>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <wait.h>
#include <array>
#include <vector>
namespace
{
	constexpr int maxClientCount = 5;
	constexpr short socketPort = 9527;
	int handleError(int socket_fd)
	{

	}
	void run_server(uint type)
	{
		int server_socket, client_socket;
		char message[1024] = {};
		sockaddr_in server_addr{}, client_addr{};
		socklen_t clientAddrLen;
		int permissionEnable = 1;
		if (type == 0)
		{
			//TCP
			server_socket = socket(PF_INET, SOCK_STREAM, 0);
			//启动复用地址权限，在服务器关闭的时候允许快速重启
			setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &permissionEnable, sizeof(permissionEnable));
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			server_addr.sin_port = htons(socketPort);
			bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
			listen(server_socket, maxClientCount);
			epoll_event listen_event{}; //epoll事件
			int epfd, event_cnt;
			epfd = epoll_create1(EPOLL_CLOEXEC);
			// 将 server_socket 这个已有的文件描述符作为用户数据保存；
			// epoll_wait 返回就绪事件时会原样带回，方便识别事件来源。
			listen_event.data.fd = server_socket;
			listen_event.events = EPOLLIN;  //关注可读事件，默认为水平触发
			epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &listen_event); //将服务器套接字加入到监控中
			std::array<epoll_event, 100> epoll_events{};
			std::vector<int> socket_fds;
			while (true)
			{
				// timeout为-1，标识阻塞等待
				int count = epoll_wait(epfd, epoll_events.data(), 100, -1);
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
						int ready_fd = epoll_events[i].data.fd;
						int ready_fd_event = epoll_events[i].events;
						if (ready_fd == server_socket)
						{
							//此时就绪的是服务器套接字，表示可以accept客户端
							client_socket = accept(server_socket, (sockaddr*)&client_addr, &clientAddrLen);

						}
						
					}
				}
			}
		}
		else if (type == 1)
		{
			//UDP

		}
		else {

		}
	}
	void run_client(uint type)
	{

		if (type == 0)
		{
			//TCP

		}
		else if (type == 1)
		{
			//UDP

		}
		else {

		}
	}
}//namespace
void MyEchoServer(const char* mode,const char* protocol)
{
	//结合epoll多线程IO实现TCP和UDP可选择的回声服务器
	if (mode == nullptr)
	{
		std::perror("Invalid input! The echo server need to input c to run client or input s to run server.");
		return;
	}
	if (strcasecmp(mode, "c") == 0)
	{
		if (strcasecmp(protocol, "TCP") == 0)
		{
			//启动TCP的客户端
			run_client(0);
		}
		else if (strcasecmp(protocol, "UDP") == 0)
		{
			//启动UDP的客户端
			run_client(1);
		}
	}
	if (strcasecmp(mode, "s") == 0)
	{
		if (strcasecmp(protocol, "TCP") == 0)
		{
			//启动TCP的服务器
			run_client(0);
		}
		else if (strcasecmp(protocol, "UDP") == 0)
		{
			//启动UDP的服务器
			run_client(1);
		}
	}
	std::perror("Invalid input! The echo server need to input c to run client or input s to run server.");
}