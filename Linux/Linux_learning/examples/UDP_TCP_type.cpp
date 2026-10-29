#include"UDP_TCP_type.h"
#include<sys/socket.h>
#include<wait.h>
#include<iostream>
void UDP_TCP_type()
{
	int tcp_socket, udp_socket;
	int optval = 0;
	socklen_t len = sizeof(optval);
	tcp_socket = socket(PF_INET, SOCK_STREAM, 0);
	udp_socket = socket(PF_INET, SOCK_DGRAM, 0);

	printf("SOCK_STREAM:%d\n", SOCK_STREAM);
	printf("SOCK_DGRAM:%d\n", SOCK_DGRAM);

	getsockopt(tcp_socket, SOL_SOCKET, SO_TYPE, (void*)&optval, &len);
	printf("tcp_sock type is :%d\n", optval);
	optval = 0;
	getsockopt(udp_socket, SOL_SOCKET, SO_TYPE, (void*)&optval, &len);
	printf("udp_socket type is :%d\n", optval);
	
	close(tcp_socket);
	close(udp_socket);
}