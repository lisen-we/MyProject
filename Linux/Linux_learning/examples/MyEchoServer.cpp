#include "MyEchoServer.h"
#include <iostream>
#include <cstdio>
#include <cstring>
#include <strings.h>
#include <cerrno>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unistd.h>
namespace
{
	constexpr int maxClientCount = 5;
	constexpr short socketPort = 9527;
	void run_server(int type)
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
			epoll_event listen_event{};
			//listen_event 相当于一个监控清单，epoll会监控清单中的事件
			//当被监控的socket触发了该事件之后会带着data.fd中的备注信息一起返回
			const int epfd = epoll_create1(EPOLL_CLOEXEC);
			// 将 server_socket 这个已有的文件描述符作为用户数据保存；
			// epoll_wait 返回就绪事件时会原样带回，方便识别事件来源。
			listen_event.data.fd = server_socket;
			listen_event.events = EPOLLIN;  //关注可读事件，默认为水平触发
			//将服务器套接字加入到监控中
			epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &listen_event);
			std::array<epoll_event, 100> epoll_events{};
			std::unordered_map<int, std::string> client_names;
			std::unordered_map<int, std::string> client_receive_buffers;
			while (true)
			{
				// timeout为-1，标识阻塞等待
				const int count = epoll_wait(epfd,
					epoll_events.data(),
					static_cast<int>(epoll_events.size()),
					-1);
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
						const int ready_fd = epoll_events[i].data.fd;
						const std::uint32_t ready_fd_events = epoll_events[i].events;
						if (ready_fd == server_socket)
						{
							// 此时就绪的是服务器套接字，表示有新的客户端连接。
							clientAddrLen = sizeof(client_addr);
							client_socket = accept(server_socket,
								reinterpret_cast<sockaddr*>(&client_addr),
								&clientAddrLen);

							epoll_event client_event{};
							client_event.data.fd = client_socket;
							client_event.events = EPOLLIN;
							// 使用epoll_ctl告诉epfd对象请把client_socket的监控配置设置为client_event
							epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &client_event);

							// 空字符串表示客户端尚未发送第一行姓名。
							client_names.emplace(client_socket, "");
							client_receive_buffers.emplace(client_socket, "");

							constexpr char name_prompt[] = "Please enter your name:\n";
							send(client_socket,
								name_prompt,
								sizeof(name_prompt) - 1,
								MSG_NOSIGNAL);
						}
						else if ((ready_fd_events & EPOLLIN) != 0)
						{
							const ssize_t received_count = read(ready_fd, message, sizeof(message));
							if (received_count <= 0)
							{
								// 在read失败或者返回0（收到FIN信号）的时候需要使用epoll_ctl删除监控socket的配置
								epoll_ctl(epfd, EPOLL_CTL_DEL, ready_fd, nullptr);
								close(ready_fd);
								client_names.erase(ready_fd);
								client_receive_buffers.erase(ready_fd);
								continue;
							}

							// TCP 是字节流。先将本次数据追加到该客户端的接收缓冲区，
							// 再按换行符拆成完整的“姓名”或“聊天消息”。
							std::string& receive_buffer = client_receive_buffers[ready_fd];
							// 使用读到的信息修改map中client套接字对应的信息
							receive_buffer.append(message, static_cast<std::size_t>(received_count));

							std::size_t line_end = receive_buffer.find('\n');
							while (line_end != std::string::npos)
							{
								std::string line = receive_buffer.substr(0, line_end);
								receive_buffer.erase(0, line_end + 1);
								if (!line.empty() && line.back() == '\r')
									line.pop_back();

								std::string& client_name = client_names[ready_fd];
								if (client_name.empty())
								{
									// 客户端连接后的第一行用于注册名称。
									client_name = line;
									const std::string joined_message =
										"[system] " + client_name + " joined the chat\n";
									for (const auto& client : client_names)
									{
										if (client.first != ready_fd)
										{
											// 这里使用 send() 而不是 write()，因为 send() 多了 flags 参数。
											// 正常关闭 TCP 连接时，客户端会向服务端发送 FIN。
											// 服务端通常在 read() 返回 0 时，才确认对端已经关闭连接。
											//
											// 在服务端处理到该断开事件之前，另一个客户端可能已经发送了消息。
											// 此时服务端进行广播，仍可能向这个即将或已经断开的客户端 socket 发送数据。
											//
											// 若不使用 MSG_NOSIGNAL，send() 失败时可能触发 SIGPIPE；
											// SIGPIPE 的默认行为会终止整个进程。
											// MSG_NOSIGNAL 会禁止这次 send() 触发 SIGPIPE，
											// 使 send() 返回 -1，并通过 errno（常见为 EPIPE 或 ECONNRESET）报告失败。
											send(client.first,
												joined_message.data(),
												joined_message.size(),
												MSG_NOSIGNAL);
										}
									}
								}
								else if (!line.empty())
								{
									const std::string forward_message =
										"[" + client_name + "] " + line + '\n';
									for (const auto& client : client_names)
									{
										if (client.first != ready_fd)
										{
											send(client.first,
												forward_message.data(),
												forward_message.size(),
												MSG_NOSIGNAL);
										}
									}
								}

								line_end = receive_buffer.find('\n');
							}
						}
					}
				}
			}
			close(epfd);
			close(server_socket);
		}
		else if (type == 1)
		{
			// UDP 没有 accept()；每个“IP:端口”组合在这里视为一个聊天客户端。
			server_socket = socket(PF_INET, SOCK_DGRAM, 0);
			setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &permissionEnable, sizeof(permissionEnable));
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
			server_addr.sin_port = htons(socketPort);
			bind(server_socket, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr));

			epoll_event udp_event{};
			udp_event.data.fd = server_socket;
			udp_event.events = EPOLLIN;
			const int udp_epfd = epoll_create1(EPOLL_CLOEXEC);
			epoll_ctl(udp_epfd, EPOLL_CTL_ADD, server_socket, &udp_event);

			struct UdpClient
			{
				sockaddr_in address;
				std::string name;
			};

			std::unordered_map<std::string, UdpClient> udp_clients;
			std::array<epoll_event, 100> udp_events{};
			while (true)
			{
				const int count = epoll_wait(udp_epfd,
					udp_events.data(),
					static_cast<int>(udp_events.size()),
					-1);
				if (count == -1)
				{
					if (errno == EINTR)
						continue;
					break;
				}

				for (int i = 0; i < count; ++i)
				{
					if (udp_events[i].data.fd != server_socket ||
						(udp_events[i].events & EPOLLIN) == 0)
					{
						continue;
					}

					char datagram[1024] = {};
					sockaddr_in sender_address{};
					socklen_t sender_address_length = sizeof(sender_address);
					const ssize_t received_count = recvfrom(server_socket,
						datagram,
						sizeof(datagram),
						0,
						reinterpret_cast<sockaddr*>(&sender_address),
						&sender_address_length);
					if (received_count <= 0)
						continue;

					char sender_ip[INET_ADDRSTRLEN] = {};
					// recvfrom填入发送方的IP地址到sin_addr,但是是二进制形式，需要使用
					// inet_ntop进行转换，将二进制转换成可读的IP地址
					// p:presentation 表演，演讲，可读
					inet_ntop(AF_INET,
						&sender_address.sin_addr,
						sender_ip,
						sizeof(sender_ip));
					// ntohs可以将传输中使用的二进制端口信息转换成可读形式
					const std::string sender_key = std::string(sender_ip) + ":" +
						std::to_string(ntohs(sender_address.sin_port));

					auto inserted = udp_clients.emplace(
						sender_key,
						UdpClient{sender_address, ""});
					UdpClient& sender = inserted.first->second;
					sender.address = sender_address;

					// 一个 UDP 数据报本身有边界：第一份数据报是姓名，之后每份是聊天消息。
					std::string content(datagram, static_cast<std::size_t>(received_count));
					while (!content.empty() &&
						(content.back() == '\n' || content.back() == '\r'))
					{
						content.pop_back();
					}

					if (sender.name.empty())
					{
						sender.name = content;
						const std::string joined_message =
							"[system] " + sender.name + " joined the chat\n";
						for (const auto& client : udp_clients)
						{
							if (client.first != sender_key)
							{
								sendto(server_socket,
									joined_message.data(),
									joined_message.size(),
									0,
									reinterpret_cast<const sockaddr*>(&client.second.address),
									sizeof(client.second.address));
							}
						}
					}
					else if (!content.empty())
					{
						const std::string forward_message =
							"[" + sender.name + "] " + content + '\n';
						for (const auto& client : udp_clients)
						{
							if (client.first != sender_key)
							{
								sendto(server_socket,
									forward_message.data(),
									forward_message.size(),
									0,
									reinterpret_cast<const sockaddr*>(&client.second.address),
									sizeof(client.second.address));
							}
						}
					}
				}
			}

			close(udp_epfd);
			close(server_socket);
		}
		else {

		}
	}
	void run_client(int type)
	{
		if (type != 0 && type != 1)
			return;

		const bool is_tcp = type == 0;
		const int socket_type = is_tcp ? SOCK_STREAM : SOCK_DGRAM;
		const int client_socket = socket(PF_INET, socket_type, 0);
		if (client_socket == -1)
		{
			std::perror("socket");
			return;
		}

		sockaddr_in server_address{};
		server_address.sin_family = AF_INET;
		server_address.sin_port = htons(socketPort);
		if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) != 1)
		{
			std::perror("inet_pton");
			close(client_socket);
			return;
		}

		// TCP 需要先建立连接；UDP 不建立连接，发送时直接指定 server_address。
		if (is_tcp && connect(client_socket,
			reinterpret_cast<const sockaddr*>(&server_address),
			sizeof(server_address)) == -1)
		{
			std::perror("connect");
			close(client_socket);
			return;
		}

		const int epfd = epoll_create1(EPOLL_CLOEXEC);
		if (epfd == -1)
		{
			std::perror("epoll_create1");
			close(client_socket);
			return;
		}

		// 客户端同时监听键盘输入和服务端发来的聊天消息。
		epoll_event input_event{};
		input_event.data.fd = STDIN_FILENO;
		input_event.events = EPOLLIN;
		epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &input_event);

		epoll_event socket_event{};
		socket_event.data.fd = client_socket;
		socket_event.events = EPOLLIN;
		epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &socket_event);

		auto send_message_to_server = [&](const std::string& message)
		{
			if (is_tcp)
			{
				// TCP 是字节流，循环发送，直到一整条换行结尾的消息都发送完。
				std::size_t sent_size = 0;
				while (sent_size < message.size())
				{
					const ssize_t sent_count = send(client_socket,
						message.data() + sent_size,
						message.size() - sent_size,
						MSG_NOSIGNAL);
					if (sent_count <= 0)
						return false;
					sent_size += static_cast<std::size_t>(sent_count);
				}
				return true;
			}

			// UDP 中一次 sendto 对应一个完整的数据报，保留消息边界。
			const ssize_t sent_count = sendto(client_socket,
				message.data(),
				message.size(),
				0,
				reinterpret_cast<const sockaddr*>(&server_address),
				sizeof(server_address));
			return sent_count == static_cast<ssize_t>(message.size());
		};

		std::cout << "[client] connected to 127.0.0.1:" << socketPort
			<< ". Please enter your name. Type /quit to exit.\n";
		std::array<epoll_event, 2> epoll_events{};
		std::string input_buffer;
		bool name_sent = false;
		bool running = true;
		while (running)
		{
			const int count = epoll_wait(epfd,
				epoll_events.data(),
				static_cast<int>(epoll_events.size()),
				-1);
			if (count == -1)
			{
				if (errno == EINTR)
					continue;
				break;
			}

			for (int i = 0; i < count && running; ++i)
			{
				const int ready_fd = epoll_events[i].data.fd;
				const std::uint32_t ready_events = epoll_events[i].events;
				if (ready_fd == STDIN_FILENO && (ready_events & EPOLLIN) != 0)
				{
					char input[1024] = {};
					const ssize_t input_count = read(STDIN_FILENO, input, sizeof(input));
					if (input_count <= 0)
					{
						running = false;
						break;
					}

					// 终端也可能一次读到多行，因此和 TCP 服务端一样按换行符拆分。
					input_buffer.append(input, static_cast<std::size_t>(input_count));
					std::size_t line_end = input_buffer.find('\n');
					while (line_end != std::string::npos)
					{
						std::string line = input_buffer.substr(0, line_end);
						input_buffer.erase(0, line_end + 1);
						if (!line.empty() && line.back() == '\r')
							line.pop_back();

						if (line == "/quit")
						{
							running = false;
							break;
						}
						if (!name_sent && line.empty())
						{
							line_end = input_buffer.find('\n');
							continue;
						}

						// 第一条消息注册姓名；之后每一行都是聊天消息。
						if (!send_message_to_server(line + '\n'))
						{
							running = false;
							break;
						}
						if (!name_sent)
						{
							name_sent = true;
							std::cout << "[client] name sent. You can start chatting.\n";
						}

						line_end = input_buffer.find('\n');
					}
				}
				else if (ready_fd == client_socket && (ready_events & EPOLLIN) != 0)
				{
					char received_message[1024] = {};
					ssize_t received_count = 0;
					if (is_tcp)
					{
						received_count = recv(client_socket,
							received_message,
							sizeof(received_message),
							0);
					}
					else
					{
						sockaddr_in sender_address{};
						socklen_t sender_address_length = sizeof(sender_address);
						received_count = recvfrom(client_socket,
							received_message,
							sizeof(received_message),
							0,
							reinterpret_cast<sockaddr*>(&sender_address),
							&sender_address_length);
					}

					if (received_count <= 0)
					{
						running = false;
						break;
					}

					std::cout.write(received_message, received_count);
					if (received_message[received_count - 1] != '\n')
						std::cout << '\n';
					std::cout.flush();
				}
			}
		}

		if (is_tcp)
			shutdown(client_socket, SHUT_WR);
		close(epfd);
		close(client_socket);
	}
}//namespace
void MyEchoServer(const char* mode, const char* protocol)
{
	if (mode == nullptr || protocol == nullptr)
	{
		std::cerr << "Usage: mode must be s or c; protocol must be TCP or UDP.\n";
		return;
	}

	int type = 0;
	if (strcasecmp(protocol, "TCP") == 0)
	{
		type = 0;
	}
	else if (strcasecmp(protocol, "UDP") == 0)
	{
		type = 1;
	}
	else
	{
		std::cerr << "Invalid protocol: use TCP or UDP.\n";
		return;
	}

	if (strcasecmp(mode, "c") == 0)
	{
		run_client(type);
		return;
	}
	if (strcasecmp(mode, "s") == 0)
	{
		run_server(type);
		return;
	}

	std::cerr << "Invalid mode: use s for server or c for client.\n";
}
