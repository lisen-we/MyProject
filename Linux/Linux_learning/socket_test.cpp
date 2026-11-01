#include "examples/SocketTestExample.h"
#include <cerrno>
#include <cstdio>
#include <string>
#include <arpa/inet.h>  //htons,htonl,ntohs,ntohl,inet_pton,inet_ntop()等函数
#include <sys/socket.h>  //socket,bind,listen,accept,connect等函数
#include <netinet/in.h>	//sockaddr_in,AF_INET等
#include <unistd.h>    //close函数
#include <iostream>
#include <cstring>
#include <sys/wait.h>
namespace
{
	// 阻塞式 write() 也可能只写入部分数据，循环可以保证整条消息写完。
	bool write_all(int fd, const char* data, std::size_t size)
	{
		std::size_t total = 0;
		while (total < size)
		{
			const ssize_t count = ::write(fd, data + total, size - total);
			if (count > 0)
			{
				total += static_cast<std::size_t>(count);
				continue;
			}
			if (count == -1 && errno == EINTR)
				continue;
			std::perror("write failed");
			return false;
		}
		return true;
	}
	void socket_test()
	{
		int sock = socket(PF_INET, SOCK_STREAM, 0);
		if (sock != -1)
		{
			struct sockaddr_in addr;
			addr.sin_family = AF_INET;
			addr.sin_addr.s_addr = inet_addr(" 192.168.0.1");
			addr.sin_port = htons(9527);
			bind(sock, (struct sockaddr*)&addr, sizeof(addr));
		}
	}
	void server_test()
	{
		int server, client;
		struct sockaddr_in seraddr, cliaddr;
		socklen_t cliaddrlen;
		const char* message = "hello world!\n";
		server = socket(PF_INET, SOCK_STREAM, 0); //TCP
		if (server < 0)
		{
			std::cout << "create socket failed!" << std::endl;
			return;
		}
		memset(&seraddr, 0, sizeof(seraddr));
		seraddr.sin_family = AF_INET;
		seraddr.sin_addr.s_addr = inet_addr("0.0.0.0");  //0.0.0.0表示监听所有IP
		seraddr.sin_port = htons(9527);
		ssize_t ret = bind(server, (struct sockaddr*)&seraddr, sizeof(seraddr));
		if (ret == -1)
		{
			std::cout << "bind failed!" << std::endl;
			close(server);
			return;
		}
		ret = listen(server, 3); //3表示队列中最多可以容纳三个client
		if (ret == -1)
		{
			std::cout << "listen failed!" << std::endl;
			close(server);
			return;
		}
		printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
		//cliaddr喝cliaddrlen是输出参数，用于在内核accept成功之后保存连接到来的客户端IP和端口
		client = accept(server, (struct sockaddr*)&cliaddr, &cliaddrlen);
		if (client == -1)
		{
			std::cout << "accept failed" << std::endl;
			close(server);
			return;
		}
		//write的返回值为实际写入的长度
		ret = write(client, message, strlen(message));
		if (ret != (ssize_t)strlen(message))
		{
			std::cout << "write failed" << std::endl;
			close(server);
			return;
		}
		close(server);
		close(client);//可以不执行，因为服务端关闭的时候客户端会自动关闭
	}
	void TCP_test()
	{
		pid_t pid = fork();
		if (pid == 0)
		{
			//开启客户端
			sleep(1);
			int client = socket(PF_INET, SOCK_STREAM, 0);
			struct sockaddr_in servaddr;
			memset(&servaddr, 0, sizeof(servaddr));
			servaddr.sin_family = AF_INET;
			servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
			servaddr.sin_port = htons(9527);
			int ret = connect(client, (struct sockaddr*)&servaddr, sizeof(servaddr));
			if (ret == 0)
			{
				printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
				char buffer[256] = "";
				read(client, buffer, sizeof(buffer));
				std::cout << buffer;
			}
			else
			{
				printf("%s(%d):%s %d\n", __FILE__, __LINE__, __FUNCTION__, ret);
			}
			close(client);
			std::cout << "client done!" << std::endl;
		}
		else if (pid > 0)
		{
			server_test();
			int status = 0;
			wait(&status);
		}
		else {
			std::cout << "fork failed!" << pid << std::endl;
		}
	}
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
		server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		server_addr.sin_port = htons(9527);
		int ret = connect(client, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if (ret == 0)
		{
			printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
			char buffer[256] = "hello,here is clinet! client num: \n";
			std::string message =
				"hello, here is client! local socket fd: " +
				std::to_string(client) +
				"\n";

			if (!write_all(client, message.data(), message.size()))
			{
				close(client);
				return -1;
			}
			memset(buffer, 0, sizeof(buffer));
			const ssize_t count = read(client, buffer, sizeof(buffer) - 1);
			if (count > 0)
			{
				buffer[count] = '\0';
				std::cout << buffer;
			}
			else if (count == -1)
			{
				std::perror("client read failed");
				close(client);
				return -1;
			}
		}
		else {
			std::perror("connect failed");
		}
		close(client);
		std::cout << "client done" << std::endl;
		return 0;
	}
	void run_server()
	{
		// 本教学服务器处理三个客户端后退出，避免示例永久阻塞在第四次 accept()。
		int server, client;
		struct sockaddr_in seraddr, cliaddr;
		server = socket(PF_INET, SOCK_STREAM, 0); //TCP
		if (server < 0)
		{
			std::cout << "create socket failed!" << std::endl;
			return;
		}
		memset(&seraddr, 0, sizeof(seraddr));
		seraddr.sin_family = AF_INET;
		seraddr.sin_addr.s_addr = inet_addr("0.0.0.0");  //0.0.0.0表示监听所有IP
		seraddr.sin_port = htons(9527);
		ssize_t ret = bind(server, (struct sockaddr*)&seraddr, sizeof(seraddr));
		if (ret == -1)
		{
			std::cout << "bind failed!" << std::endl;
			close(server);
			return;
		}
		ret = listen(server, 3); //3表示队列中最多可以容纳三个client
		if (ret == -1)
		{
			std::cout << "listen failed!" << std::endl;
			close(server);
			return;
		}
		printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
		char client_message[4098];
		// 教学示例与子进程创建的客户端数量保持一致，处理三个连接后正常退出。
		for (int handled_count = 0; handled_count < 3; ++handled_count)
		{
			memset(client_message, 0, sizeof(client_message));
			socklen_t cliaddrlen = sizeof(cliaddr);
			//cliaddr和cliaddrlen是输出参数，用于在内核accept成功之后保存连接到来的客户端IP和端口
			client = accept(server, (struct sockaddr*)&cliaddr, &cliaddrlen);
			if (client == -1)
			{
				std::cout << "accept failed" << std::endl;
				close(client);
				close(server);
				return;
			}
			//read将客户端发送过来的消息写入到缓冲区并输出
			ret = read(client, client_message, sizeof(client_message) - 1);
			if (ret > 0)
			{
				client_message[ret] = '\0';
				std::cout << client_message << std::endl;
			}
			if (ret == -1)
			{
				std::perror("read failed");
				close(client);
				close(server);
				return;
			}
			//write向客户端写入信息，告诉自己已经收到了其发送过来的数据
			const char* message = "receive succeeded!\n";
			if (!write_all(client, message, std::strlen(message)))
			{
				close(server);
				return;
			}
			close(client);//关闭当前已连接 Socket；监听 Socket 仍用于接收后续连接。
		}
		close(server);
	}
	void TCPV1()
	{
		pid_t pid = fork();
		if (pid == 0)
		{
			for (int i = 0; i < 3; i++)
			{
				sleep(1);
				if (run_client() != 0)
					break;
			}
		}
		else if (pid > 0)
		{
			//开启服务器，监听多个客户端发送来的消息，并打印输出。
			run_server();
			int status = 0;
			wait(&status);
		}
		else {
			std::cout << "fork failed!" << pid << std::endl;
		}
	}
}
