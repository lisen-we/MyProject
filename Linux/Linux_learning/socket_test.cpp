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
void server_test()
{
	int server, client;
	struct sockaddr_in seraddr,cliaddr;
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
			printf("%s(%d):%s %d\n", __FILE__, __LINE__, __FUNCTION__,ret);
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