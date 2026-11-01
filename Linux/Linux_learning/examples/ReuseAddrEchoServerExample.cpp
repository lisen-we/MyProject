#include "ReuseAddrEchoServerExample.h"

#include <arpa/inet.h>
#include <cerrno>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
	constexpr int kReuseAddrEchoPort = 9528;

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

	void notify_parent(int pipe_fd, char status)
	{
		while (::write(pipe_fd, &status, sizeof(status)) == -1 && errno == EINTR)
		{
		}
	}

	int run_server_once(int ready_pipe_fd, int round)
	{
		const int server = ::socket(AF_INET, SOCK_STREAM, 0);
		if (server == -1)
		{
			std::perror("server socket failed");
			notify_parent(ready_pipe_fd, 'E');
			return -1;
		}

		int reuse_address = 1;
		if (::setsockopt(server, SOL_SOCKET, SO_REUSEADDR,
			&reuse_address, sizeof(reuse_address)) == -1)
		{
			std::perror("setsockopt SO_REUSEADDR failed");
			::close(server);
			notify_parent(ready_pipe_fd, 'E');
			return -1;
		}

		struct sockaddr_in server_address;
		std::memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		server_address.sin_port = htons(kReuseAddrEchoPort);

		if (::bind(server, reinterpret_cast<struct sockaddr*>(&server_address),
			sizeof(server_address)) == -1)
		{
			std::perror("bind failed");
			::close(server);
			notify_parent(ready_pipe_fd, 'E');
			return -1;
		}

		if (::listen(server, 1) == -1)
		{
			std::perror("listen failed");
			::close(server);
			notify_parent(ready_pipe_fd, 'E');
			return -1;
		}

		std::cout << "第 " << round
			<< " 轮服务端：已在 bind() 前设置 SO_REUSEADDR，监听 127.0.0.1:"
			<< kReuseAddrEchoPort << '\n' << std::flush;
		notify_parent(ready_pipe_fd, 'R');

		const int client = ::accept(server, nullptr, nullptr);
		if (client == -1)
		{
			std::perror("accept failed");
			::close(server);
			return -1;
		}

		char message[256];
		const ssize_t read_count = ::read(client, message, sizeof(message));
		if (read_count <= 0)
		{
			if (read_count == -1)
				std::perror("server read failed");
			::close(client);
			::close(server);
			return -1;
		}

		if (!write_all(client, message, static_cast<std::size_t>(read_count)))
		{
			::close(client);
			::close(server);
			return -1;
		}

		// 服务端主动关闭已连接 Socket，使后续立即重启的场景更接近实际服务重启。
		// The server is the active closer.  Draining the peer's FIN makes this
		// endpoint enter TIME_WAIT, which is the restart condition we demonstrate.
		if (::shutdown(client, SHUT_WR) == -1)
		{
			std::perror("server shutdown failed");
			::close(client);
			::close(server);
			return -1;
		}

		char discard[256];
		for (;;)
		{
			const ssize_t drain_count = ::read(client, discard, sizeof(discard));
			if (drain_count == 0)
				break;
			if (drain_count == -1 && errno == EINTR)
				continue;
			if (drain_count == -1)
			{
				std::perror("server drain failed");
				::close(client);
				::close(server);
				return -1;
			}
		}

		::close(client);
		::close(server);
		return 0;
	}

	bool probe_bind_without_reuseaddr()
	{
		const int probe_socket = ::socket(AF_INET, SOCK_STREAM, 0);
		if (probe_socket == -1)
		{
			std::perror("probe socket failed");
			return false;
		}

		struct sockaddr_in probe_address;
		std::memset(&probe_address, 0, sizeof(probe_address));
		probe_address.sin_family = AF_INET;
		probe_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		probe_address.sin_port = htons(kReuseAddrEchoPort);

		// Deliberately do not call setsockopt(..., SO_REUSEADDR, ...).
		if (::bind(probe_socket, reinterpret_cast<struct sockaddr*>(&probe_address),
			sizeof(probe_address)) == 0)
		{
			std::cout << "【对照结果】未设置 SO_REUSEADDR 的 bind() 意外成功；"
				"当前系统未能观察到 TIME_WAIT 对重启的限制。\n";
			::close(probe_socket);
			return false;
		}

		const int bind_error = errno;
		::close(probe_socket);
		if (bind_error == EADDRINUSE)
		{
			std::cout << "【对照结果】未设置 SO_REUSEADDR 的 bind() 按预期失败："
				"EADDRINUSE（Address already in use）。\n";
			return true;
		}

		errno = bind_error;
		std::perror("bind without SO_REUSEADDR failed for an unexpected reason");
		return false;
	}

	bool run_client_once(const std::string& message, int round)
	{
		const int client = ::socket(AF_INET, SOCK_STREAM, 0);
		if (client == -1)
		{
			std::perror("client socket failed");
			return false;
		}

		struct sockaddr_in server_address;
		std::memset(&server_address, 0, sizeof(server_address));
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(kReuseAddrEchoPort);
		server_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		if (::connect(client, reinterpret_cast<struct sockaddr*>(&server_address),
			sizeof(server_address)) == -1)
		{
			std::perror("connect failed");
			::close(client);
			return false;
		}

		if (!write_all(client, message.data(), message.size()))
		{
			::close(client);
			return false;
		}

		char echoed_message[256];
		const ssize_t read_count = ::read(client, echoed_message, sizeof(echoed_message));
		if (read_count > 0)
		{
			std::cout << "第 " << round << " 轮客户端收到回声："
				<< std::string(echoed_message, static_cast<std::size_t>(read_count)) << '\n';
		}
		else if (read_count == -1)
		{
			std::perror("client read failed");
			::close(client);
			return false;
		}
		else
		{
			::close(client);
			return false;
		}

		// Consume the server's FIN before closing this endpoint.  The server then
		// becomes the active closer and owns the TIME_WAIT state for this connection.
		char discard[256];
		for (;;)
		{
			const ssize_t drain_count = ::read(client, discard, sizeof(discard));
			if (drain_count == 0)
				break;
			if (drain_count == -1 && errno == EINTR)
				continue;
			if (drain_count == -1)
			{
				std::perror("client drain failed");
				::close(client);
				return false;
			}
		}

		::close(client);
		return true;
	}

	bool run_echo_round(int round)
	{
		int ready_pipe[2];
		if (::pipe(ready_pipe) == -1)
		{
			std::perror("pipe failed");
			return false;
		}

		const pid_t server_pid = ::fork();
		if (server_pid == -1)
		{
			std::perror("fork failed");
			::close(ready_pipe[0]);
			::close(ready_pipe[1]);
			return false;
		}

		if (server_pid == 0)
		{
			::close(ready_pipe[0]);
			const int result = run_server_once(ready_pipe[1], round);
			::close(ready_pipe[1]);
			::_exit(result == 0 ? 0 : 1);
		}

		::close(ready_pipe[1]);
		char server_status = 'E';
		const ssize_t status_count = ::read(ready_pipe[0], &server_status, sizeof(server_status));
		::close(ready_pipe[0]);

		bool client_succeeded = false;
		if (status_count == 1 && server_status == 'R')
		{
			client_succeeded = run_client_once("SO_REUSEADDR restart demo", round);
		}
		else
		{
			std::cerr << "第 " << round << " 轮服务端未能启动，无法创建客户端连接。\n";
		}

		int wait_status = 0;
		while (::waitpid(server_pid, &wait_status, 0) == -1 && errno == EINTR)
		{
		}
		return client_succeeded && WIFEXITED(wait_status) && WEXITSTATUS(wait_status) == 0;
	}
}

void ReuseAddrEchoServerExample()
{
	std::cout << "【SO_REUSEADDR TCP 回声服务重启示例】\n";
	std::cout << "场景：服务端主动关闭一个 TCP 连接后，立刻重启并继续使用同一端口。\n";
	std::cout << "预期：不设置 SO_REUSEADDR 的 bind() 遇到 TIME_WAIT 将报 EADDRINUSE；"
		"设置后可立即重启。\n";

	if (!run_echo_round(1))
		return;

	std::cout << "【对照】第 1 轮服务端已退出。现在立即用未设置 SO_REUSEADDR 的 Socket "
		"对同一端口调用 bind()……\n";
	const bool without_reuseaddr_failed = probe_bind_without_reuseaddr();

	if (!run_echo_round(2))
		return;

	if (without_reuseaddr_failed)
	{
		std::cout << "【结论】同一端口的无选项 bind() 被拒绝后，设置 SO_REUSEADDR 的"
			"第 2 轮服务端仍可立即启动。\n";
	}
	else
	{
		std::cout << "【提示】第 2 轮服务端已用 SO_REUSEADDR 成功重启，但当前系统"
			"没有复现未设置选项时的 TIME_WAIT 失败。\n";
	}

	std::cout << "注意：SO_REUSEADDR 不能抢占仍由其他进程监听的端口。\n";
}
