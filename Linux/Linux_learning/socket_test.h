#pragma once
#include<arpa/inet.h>  //htons,htonl,ntohs,ntohl,inet_pton,inet_ntop()等函数
#include<sys/socket.h>  //socket,bind,listen,accept,connect等函数
#include<netinet/in.h>	//sockaddr_in,AF_INET等
void socket_test();