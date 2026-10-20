#include "CalculationServer.h"
#include<sys/unistd.h>
#include<sys/wait.h>
#include<cerrno>
#include<cstdio>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<cstring>
#include<iostream>
#include<thread>
#include<atomic>
#include<stack>
#include<sstream>
namespace
{
	std::atomic<bool> server_status{ true };
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
				// 服务器返回中缀表达式的计算结果。
				char calculate_result[4096];
				memset(calculate_result, 0, sizeof(calculate_result));
				//简单版本，后续会使用注释提供循环读取功能
				ret = read(client, calculate_result, sizeof(calculate_result) - 1);
				if (ret > 0)
				{
					calculate_result[ret] = '\0';
					// 输入消息已包含 '\n'，因此只输出一次，不再追加 std::endl。
					std::cout << "服务器返回结果：" << calculate_result << std::flush;
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
	std::string changeInFixToPostFix(const std::string& expression)
	{
		auto isOperator = [](char value)
			{
				return value == '+' || value == '-' ||
					value == '*' || value == '/';
			};

		auto precedence = [](char value)
			{
				if (value == '+' || value == '-')
					return 1;

				if (value == '*' || value == '/')
					return 2;

				return 0;
			};

		std::string postfix;
		std::stack<char> operators;
		bool expect_operand = true;
		bool has_operand = false;

		auto appendToken = [&postfix](const std::string& token)
			{
				if (!postfix.empty())
					postfix += ' ';

				postfix += token;
			};

		for (std::size_t index = 0; index < expression.size();)
		{
			const char current = expression[index];

			if (std::isspace(static_cast<unsigned char>(current)))
			{
				++index;
				continue;
			}

			// 找到当前字符后的下一个非空白字符，用于判断 -(...)。
			std::size_t next = index + 1;
			while (next < expression.size() &&
				std::isspace(static_cast<unsigned char>(expression[next])))
			{
				++next;
			}

			// -(...) 转换为 0 - (...)；+(...) 可直接忽略。
			if (expect_operand &&
				(current == '+' || current == '-') &&
				next < expression.size() &&
				expression[next] == '(')
			{
				if (current == '+')
				{
					++index;
					continue;
				}

				appendToken("0");
				has_operand = true;
				expect_operand = false;
				// 继续向下按二元 '-' 处理。
			}

			const bool signed_number =
				expect_operand &&
				(current == '+' || current == '-') &&
				index + 1 < expression.size() &&
				(std::isdigit(static_cast<unsigned char>(expression[index + 1])) ||
					expression[index + 1] == '.');

			const bool number_starts =
				std::isdigit(static_cast<unsigned char>(current)) ||
				current == '.' ||
				signed_number;

			if (number_starts)
			{
				if (!expect_operand)
					return "ERR missing operator before number";

				try
				{
					std::size_t parsed_length = 0;
					const std::string remaining = expression.substr(index);

					// 只借助 stod 确定一个合法数字占用多少字符。
					std::stod(remaining, &parsed_length);

					if (parsed_length == 0)
						return "ERR invalid number";

					appendToken(remaining.substr(0, parsed_length));
					index += parsed_length;
					expect_operand = false;
					has_operand = true;
					continue;
				}
				catch (...)
				{
					return "ERR invalid number";
				}
			}

			if (current == '(')
			{
				if (!expect_operand)
					return "ERR missing operator before '('";

				operators.push(current);
				++index;
				continue;
			}

			if (current == ')')
			{
				if (expect_operand)
					return "ERR missing operand before ')'";

				bool matched = false;
				while (!operators.empty())
				{
					if (operators.top() == '(')
					{
						operators.pop();
						matched = true;
						break;
					}

					appendToken(std::string(1, operators.top()));
					operators.pop();
				}

				if (!matched)
					return "ERR unmatched ')'";

				++index;
				expect_operand = false;
				continue;
			}

			if (isOperator(current))
			{
				if (expect_operand)
					return "ERR operator without left operand";

				while (!operators.empty() &&
					operators.top() != '(' &&
					precedence(operators.top()) >= precedence(current))
				{
					appendToken(std::string(1, operators.top()));
					operators.pop();
				}

				operators.push(current);
				++index;
				expect_operand = true;
				continue;
			}

			return "ERR invalid character";
		}

		if (!has_operand)
			return "ERR empty expression";

		if (expect_operand)
			return "ERR expression ends with operator";

		while (!operators.empty())
		{
			if (operators.top() == '(')
				return "ERR unmatched '('";

			appendToken(std::string(1, operators.top()));
			operators.pop();
		}

		return postfix;
	}
	std::string calculatePostfixExpression(const std::string& expression)
	{
		std::istringstream input(expression);
		std::stack<double> values;
		std::string token;

		while (input >> token)
		{
			const bool is_operator =
				token.size() == 1 &&
				(token[0] == '+' || token[0] == '-' ||
					token[0] == '*' || token[0] == '/');

			if (!is_operator)
			{
				try
				{
					std::size_t parsed_length = 0;
					const double number = std::stod(token, &parsed_length);

					// 例如 "12abc" 不能作为合法数字。
					if (parsed_length != token.size())
						return "ERR invalid number: " + token;

					values.push(number);
				}
				catch (...)
				{
					return "ERR invalid token: " + token;
				}

				continue;
			}

			// 一个二元运算符至少需要两个操作数。
			if (values.size() < 2)
				return "ERR insufficient operands";

			const double right = values.top();
			values.pop();

			const double left = values.top();
			values.pop();

			double result = 0.0;
			switch (token[0])
			{
			case '+':
				result = left + right;
				break;
			case '-':
				result = left - right;
				break;
			case '*':
				result = left * right;
				break;
			case '/':
				if (right == 0.0)
					return "ERR division by zero";

				result = left / right;
				break;
			}

			values.push(result);
		}

		// 正确的后缀表达式最后只能留下一个结果。
		if (values.size() != 1)
			return "ERR invalid postfix expression";

		std::ostringstream output;
		output << "OK " << values.top();
		return output.str();
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
			//服务器循环
			struct sockaddr_in client_addr;
			socklen_t client_addr_len = sizeof(client_addr);
			int client = accept(server, (struct sockaddr*)&client_addr, &client_addr_len);
			if (client == -1)
			{
				std::cout << "accept failed" << std::endl;
				continue;
			}
			std::string receive_message;
			// 同一个 TCP 连接可以连续发送多条消息；直到客户端断开后才接收下一个客户端。
			while (true)
			{
				memset(client_message, 0, sizeof(client_message));
				ret = read(client, client_message, sizeof(client_message));
				if (ret > 0)
				{
					receive_message.append(client_message, static_cast<std::size_t>(ret));
					std::size_t end = 0;
					while ((end = receive_message.find('\n')) != std::string::npos)
					{
						//一次read中有多条客户端数据的情况，根据'\n'来进行切割
						std::string infix_expression = receive_message.substr(0, end);
						receive_message.erase(0, end + 1);
						std::string post_expression = changeInFixToPostFix(infix_expression);
						std::string result = post_expression.rfind("ERR ", 0) == 0
							? post_expression
							: calculatePostfixExpression(post_expression);
						result.push_back('\n');
						//将得到的result值写回到客户端
						if (!write_all(client, result.data(), static_cast<std::size_t>(result.size())))
							continue;
					}
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
void calculationServerTest()
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
