#include <sys/socket.h>
#include <sys/wait.h>
#include "examples/EchoServerExample.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cerrno>
std::atomic<bool> server_status{ true };
namespace
{
	bool write_all(int fd, const char* data, std::size_t size)
	{
		std::size_t total = 0;
		while (total < size)
		{
			const ssize_t count = write(fd, data + total, size - total);
			if (count > 0)
			{
				total += static_cast<std::size_t>(count);
				continue;
			}
			if (count == -1 && errno == EINTR)
				continue;
			std::perror("write failed!");
			return false;
		}
		return true;
	}
	//回声服务器
	int run_client()
	{
		int client = socket(PF_INET, SOCK_STREAM, 0);
		if (client == -1)
		{
			std::perror("socket failed!");
			return -1;
		}
		struct sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(9527);
		server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		int ret = connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if (ret == 0)
		{
			//连接服务器成功
			std::cout << "请输入内容，输入 quit 退出：\n";
			std::string input;
			while (true)
			{
				std::cout << "> " << std::flush;
				if (!std::getline(std::cin, input))
				{
					// Linux 终端通常可以按 Ctrl+D 产生 EOF。
					std::cout << "\n终端输入结束\n";
					break;
				}
				if (input == "quit")
				{
					std::cout << "客户端准备退出\n";
					break;
				}
				// getline() 会删除输入末尾的换行符，这里重新添加，
				// 让服务端可以使用换行符识别一条消息的结束位置。
				input.push_back('\n');
				if (!write_all(client, input.data(), input.size()))
				{
					return -1;
				}
				// 回声服务器应当返回相同数量的字节。
				char echoed_data[4096];
				memset(echoed_data, 0, sizeof(echoed_data));
				//简单版本，后续会使用注释提供循环读取功能
				ret = read(client, echoed_data, sizeof(echoed_data) - 1);
				if (ret > 0)
				{
					echoed_data[ret] = '\0';
					// 输入消息已包含 '\n'，因此只输出一次，不再追加 std::endl。
					std::cout << "服务器回声：" << echoed_data << std::flush;
				}
				if (ret == -1)
				{
					std::perror("read failed");
					close(client);
					return -1;
				}
			}
			// 告诉服务器：客户端不会继续发送，但仍允许接收剩余数据。
			if (shutdown(client, SHUT_WR) == -1)
			{
				std::perror("shutdown failed");
				return -1;
			}
		}
	}
	int run_server()
	{
		int server = socket(PF_INET, SOCK_STREAM, 0);
		struct sockaddr_in server_addr;
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(9527);
		server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
		ssize_t ret = bind(server, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if (ret == -1)
		{
			std::cout << "bind failed!" << std::endl;
			close(server);
			return -1;
		}
		ret = listen(server, 3); //3表示队列中最多可以容纳三个client
		if (ret == -1)
		{
			std::cout << "listen failed!" << std::endl;
			close(server);
			return -1;
		}
		std::thread timer([&]
			{
				std::this_thread::sleep_for(std::chrono::seconds(30));

				server_status = false;

			});
		//使用定时器让回声服务器开启30秒，只处理30秒内来自客户端的信息
		char client_message[4096];
		while (server_status)
		{
			//回声服务器循环
			struct sockaddr_in client_addr;
			socklen_t client_addr_len = sizeof(client_addr);
			int client = accept(server, (struct sockaddr*)&client_addr, &client_addr_len);
			if (client == -1)
			{
				std::cout << "accept failed" << std::endl;
				continue;
			}
			// 同一个 TCP 连接可以连续发送多条消息；直到客户端断开后才接收下一个客户端。
			while (true)
			{
				memset(client_message, 0, sizeof(client_message));
				ret = read(client, client_message, sizeof(client_message));
				if (ret > 0)
				{
					// 按实际接收的字节数回写，不能依赖 C 字符串的结尾标记。
					if (!write_all(client, client_message, static_cast<std::size_t>(ret)))
						break;
					continue;
				}

				if (ret == 0)
					break;

				if (errno == EINTR)
					continue;

				std::perror("read failed");
				break;
			}
			close(client);
		}
		close(server);
	}
}

// EchoServe 是供 main.cpp 调用的公开入口，不能放在匿名命名空间中。
void EchoServe()
{
	pid_t pid = fork();
	if (pid == 0)
	{
		sleep(1);
		//连接成功，进入子进程分支
		if (!run_client())
			return;
	}
	else if (pid > 0)
	{
		//主进程分支
		if (!run_server())
		{
			int status = 0;
			wait(&status);
		}
	}
	else
	{
		std::perror("fork failed!");
	}
}
