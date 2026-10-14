#include "socket_test.h"
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