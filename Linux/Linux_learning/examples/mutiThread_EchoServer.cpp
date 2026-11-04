#include "mutiThread_EchoServer.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
namespace
{
	int clit_socks[100] = { 0 };
	int clit_cnt = 0;
	pthread_mutex_t mutex;
	void send_msg(const char* msg,int str_len)
	{
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < clit_cnt; i++)
		{
			if(clit_socks[i]>=0)
				write(clit_socks[i], msg, str_len);
		}
		pthread_mutex_unlock(&mutex);
	}
	void* handle_cnt(void* arg)
	{
		pthread_detach(pthread_self());
		int clit_sock = *(int*)arg;
		char msg[1024] = "";
		int str_len = read(clit_sock, msg, sizeof(msg));
		send_msg(msg, str_len);
		pthread_mutex_lock(&mutex);
		for (int i = 0; i < clit_cnt; i++)
		{
			if (clit_socks[i] == clit_sock)
			{
				clit_socks[i] = -1;
				break;
			}
		}
		pthread_mutex_unlock(&mutex);
		close(clit_sock);
		pthread_exit(NULL);
	}
	void run_server()
	{
		struct sockaddr_in servaddr;
		struct sockaddr_in clitaddr;
		socklen_t clitaddrlen;
		int server_socket = socket(PF_INET, SOCK_STREAM, 0);
		memset(&servaddr, 0, sizeof(servaddr));
		servaddr.sin_family = AF_INET;
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servaddr.sin_port = htons(10000);
		pthread_mutex_init(&mutex,NULL);
		if (bind(server_socket, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
		{
			std::perror("bind failed");
			return;
		}
		if (listen(server_socket, 5) == -1)
		{
			std::perror("listen failed");
			return;
		}
		for (;;)//while(1)
		{
			int client_socket = accept(server_socket, (struct sockaddr*)&clitaddr, &clitaddrlen);
			if (client_socket == -1)
			{
				std::perror("accept error");
				break;
			}
			pthread_mutex_lock(&mutex);
			clit_socks[clit_cnt++] = client_socket;
			pthread_mutex_unlock(&mutex);
			pthread_t thread;
			pthread_create(&thread, NULL, handle_cnt, &clit_socks[clit_cnt - 1]);
		}
		close(server_socket);
		pthread_mutex_destroy(&mutex);
	}
}//namespace
void mutiThread_EchoServer()
{

}