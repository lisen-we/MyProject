#include "Pipe_test.h"

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <poll.h>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace
{
	constexpr auto kChildWriteDelay = std::chrono::seconds(4);
	constexpr auto kParentWaitLimit = std::chrono::seconds(6);
	constexpr int kPollingIntervalMilliseconds = 1000;

	bool write_all(int file_descriptor, const char* data, std::size_t size)
	{
		std::size_t written_total = 0;
		while (written_total < size)
		{
			const ssize_t written_count =
				::write(file_descriptor, data + written_total, size - written_total);
			if (written_count > 0)
			{
				written_total += static_cast<std::size_t>(written_count);
				continue;
			}
			if (written_count == -1 && errno == EINTR)
				continue;

			std::perror("pipe write failed");
			return false;
		}
		return true;
	}

	ssize_t read_retry(int file_descriptor, char* buffer, std::size_t size)
	{
		ssize_t read_count = -1;
		do
		{
			read_count = ::read(file_descriptor, buffer, size);
		} while (read_count == -1 && errno == EINTR);
		return read_count;
	}
}

void singlePipeTest()
{
	int pipe_fd[2] = {-1, -1};
	if (::pipe(pipe_fd) == -1)
	{
		std::perror("pipe failed");
		return;
	}

	const pid_t child_pid = ::fork();
	if (child_pid == -1)
	{
		std::perror("fork failed");
		::close(pipe_fd[0]);
		::close(pipe_fd[1]);
		return;
	}

	if (child_pid == 0)
	{
		// The child only writes to the pipe.
		::close(pipe_fd[0]);
		std::this_thread::sleep_for(kChildWriteDelay);

		constexpr char message[] = "This is child!";
		const ssize_t written_count =
			::write(pipe_fd[1], message, sizeof(message) - 1);
		if (written_count != static_cast<ssize_t>(sizeof(message) - 1))
			std::perror("child write failed");

		::close(pipe_fd[1]);
		_exit(written_count == static_cast<ssize_t>(sizeof(message) - 1)
			? EXIT_SUCCESS : EXIT_FAILURE);
	}

	// The parent only reads from the pipe. Closing this write end is required for
	// correct EOF behaviour after the child closes its write end.
	::close(pipe_fd[1]);

	const auto deadline = std::chrono::steady_clock::now() + kParentWaitLimit;
	bool received_message = false;
	while (std::chrono::steady_clock::now() < deadline)
	{
		pollfd read_event {};
		read_event.fd = pipe_fd[0];
		read_event.events = POLLIN;

		int poll_result = -1;
		do
		{
			poll_result = ::poll(&read_event, 1, kPollingIntervalMilliseconds);
		} while (poll_result == -1 && errno == EINTR);

		if (poll_result == 0)
		{
			std::cout << "I am waiting\n";
			continue;
		}
		if (poll_result == -1)
		{
			std::perror("poll failed");
			break;
		}

		if ((read_event.revents & (POLLIN | POLLHUP)) != 0)
		{
			char buffer[100] {};
			const ssize_t read_count = ::read(pipe_fd[0], buffer, sizeof(buffer));
			if (read_count > 0)
			{
				std::cout << "parent received: "
					<< std::string(buffer, static_cast<std::size_t>(read_count)) << '\n';
				received_message = true;
				break;
			}
			if (read_count == 0)
			{
				std::cout << "pipe reached EOF before a message was received\n";
				break;
			}
			if (errno != EINTR)
			{
				std::perror("parent read failed");
				break;
			}
		}
	}

	if (!received_message)
		std::cout << "parent timed out while waiting for the child message\n";

	::close(pipe_fd[0]);

	int child_status = 0;
	while (::waitpid(child_pid, &child_status, 0) == -1 && errno == EINTR)
	{
	}
}

void DoublePipeTest()
{
	int parent_to_child[2] = {-1, -1};
	int child_to_parent[2] = {-1, -1};
	if (::pipe(parent_to_child) == -1 || ::pipe(child_to_parent) == -1)
	{
		std::perror("pipe failed");
		if (parent_to_child[0] != -1)
			::close(parent_to_child[0]);
		if (parent_to_child[1] != -1)
			::close(parent_to_child[1]);
		if (child_to_parent[0] != -1)
			::close(child_to_parent[0]);
		if (child_to_parent[1] != -1)
			::close(child_to_parent[1]);
		return;
	}

	const pid_t child_pid = ::fork();
	if (child_pid == -1)
	{
		std::perror("fork failed");
		::close(parent_to_child[0]);
		::close(parent_to_child[1]);
		::close(child_to_parent[0]);
		::close(child_to_parent[1]);
		return;
	}

	if (child_pid == 0)
	{
		// Child reads requests from parent_to_child and writes responses to child_to_parent.
		::close(parent_to_child[1]);
		::close(child_to_parent[0]);

		char request_buffer[128] {};
		const ssize_t request_count =
			read_retry(parent_to_child[0], request_buffer, sizeof(request_buffer));
		if (request_count <= 0)
		{
			if (request_count == -1)
				std::perror("child read failed");
			::close(parent_to_child[0]);
			::close(child_to_parent[1]);
			_exit(EXIT_FAILURE);
		}

		const std::string response = "child received: " +
			std::string(request_buffer, static_cast<std::size_t>(request_count));
		const bool response_sent =
			write_all(child_to_parent[1], response.data(), response.size());

		::close(parent_to_child[0]);
		::close(child_to_parent[1]);
		_exit(response_sent ? EXIT_SUCCESS : EXIT_FAILURE);
	}

	// Parent writes only to parent_to_child and reads only from child_to_parent.
	::close(parent_to_child[0]);
	::close(child_to_parent[1]);

	constexpr char request[] = "hello from parent";
	const bool request_sent =
		write_all(parent_to_child[1], request, sizeof(request) - 1);
	// Closing the write end tells the child that no further requests will arrive.
	::close(parent_to_child[1]);

	char response_buffer[256] {};
	const ssize_t response_count = request_sent
		? read_retry(child_to_parent[0], response_buffer, sizeof(response_buffer))
		: -1;
	if (response_count > 0)
	{
		std::cout << "parent received response: "
			<< std::string(response_buffer, static_cast<std::size_t>(response_count)) << '\n';
	}
	else if (response_count == 0)
	{
		std::cout << "child closed the response pipe without sending a response\n";
	}
	else if (request_sent)
	{
		std::perror("parent read response failed");
	}

	::close(child_to_parent[0]);

	int child_status = 0;
	while (::waitpid(child_pid, &child_status, 0) == -1 && errno == EINTR)
	{
	}
}
