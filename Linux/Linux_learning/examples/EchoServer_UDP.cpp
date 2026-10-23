#include"EchoServer_UDP.h"
#include<iostream>
#include<sys/socket.h>   //socket
#include<netinet/in.h>  //sockaddr_in
#include<wait.h>   //close
#include<arpa/inet.h> //inet_addr
#include<cstring>
namespace
{
	void handle_error(const char* message)
	{
		fputs(message, stderr);
		fputc('\n', stderr);
		exit(-1);
	}
	int run_server(int argc, char* argv[])
	{
		int ser_sock = -1;
		char message[512] = "";
		struct sockaddr_in servaddr;
		struct sockaddr_in clientaddr;
		socklen_t clientaddr_len;
		if (argc != 2)
		{
			printf("usage:%s<port>\n", argv[0]);
			handle_error("argument is error:");
		}
		ser_sock = socket(AF_INET, SOCK_DGRAM, 0); //UDP
		if (ser_sock == -1)
		{
			handle_error("create socket failed:");
		}

		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  //0.0.0.0
		servaddr.sin_port = htons(atoi(argv[1]));

		if (bind(ser_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		{
			handle_error("bind failed");
		}
		for (int i = 0; i < 10; i++)
		{
			clientaddr_len = sizeof(clientaddr);
			ssize_t len = recvfrom(
				ser_sock,
				message,
				sizeof(message),
				0,
				(struct sockaddr*)&clientaddr,
				&clientaddr_len
			);
			sendto(
				ser_sock,
				message,
				len,
				0,
				(struct sockaddr*)&clientaddr,
				clientaddr_len
			);
		}
		//服务端会一直占用接口直到10次循环结束后退出
		close(ser_sock);
		return 0;
	}

	int run_client(int argc, char* argv[])
	{
		int client_sock;
		struct sockaddr_in serv_addr;
		socklen_t serv_len = sizeof(serv_addr);
		char message[512] = "";
		if (argc != 3)
		{
			printf("usage:%s ip port\n", argv[0]);
			handle_error("argument error");
		}

		client_sock = socket(PF_INET, SOCK_DGRAM, 0);
		if (client_sock == -1)
			handle_error("socket creates failed");

		memset(&serv_addr, 0, sizeof(serv_addr));
		serv_addr.sin_family = PF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
		serv_addr.sin_port = htons((short)atoi(argv[2]));
		
		while (1)
		{
			printf("input message(q to Quit)");
			scanf("%s", message);
			if ((strcmp(message, "q") == 0) || (strcmp(message, "Q") == 0))
				break;
			sendto(client_sock, message, strlen(message), 0,
				(sockaddr*)&serv_addr, serv_len);
			recvfrom(client_sock, message, sizeof(message), 0, (sockaddr*)&serv_addr, &serv_len);
			printf("receive message:%s\n", message);
		}
		close(client_sock);
	}
}
void EchoServeUDP()
{
	char program_name[] = "EchoServer_UDP";
	if (fork() > 0)
	{
		//主进程使用客户端
		int argc = 3;
		char* argv[] = {
			program_name,
			(char*)"127.0.0.1",
			(char*)"9999"
		};
		run_client(argc, argv);
		int status = 0;
		wait(&status);
	}
	else
	{
		//子进程
		int argc = 2;
		char* argv[] = {
			program_name,
			(char*)"9999"
		};
		run_server(argc,argv);
	}
}
