#include "SignalThreadExample.h"

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <pthread.h>
#include <unistd.h>

namespace
{
	constexpr unsigned int kAlarmIntervalSeconds = 2;
	constexpr sig_atomic_t kMaximumAlarmCount = 3;

	volatile sig_atomic_t g_alarm_count = 0;
	volatile sig_atomic_t g_stop_requested = 0;

	void signal_handler(int signal_number)
	{
		// A signal handler must use only async-signal-safe operations. write() and
		// assignments to sig_atomic_t meet that requirement; iostream/printf/exit do not.
		if (signal_number == SIGALRM)
		{
			++g_alarm_count;
			constexpr char message[] = "signal handler: SIGALRM received\n";
			(void)::write(STDOUT_FILENO, message, sizeof(message) - 1);
			return;
		}

		if (signal_number == SIGINT)
		{
			g_stop_requested = 1;
			constexpr char message[] = "signal handler: SIGINT received, stopping...\n";
			(void)::write(STDOUT_FILENO, message, sizeof(message) - 1);
		}
	}

	bool install_handler(int signal_number, struct sigaction* previous_action)
	{
		struct sigaction action;
		std::memset(&action, 0, sizeof(action));
		action.sa_handler = signal_handler;
		if (::sigemptyset(&action.sa_mask) == -1)
		{
			std::perror("sigemptyset failed");
			return false;
		}

		action.sa_flags = SA_RESTART;
		if (::sigaction(signal_number, &action, previous_action) == -1)
		{
			std::perror("sigaction failed");
			return false;
		}
		return true;
	}

	unsigned long current_thread_id()
	{
		// pthread_t is an unsigned long on the Linux target used by this project.
		return static_cast<unsigned long>(::pthread_self());
	}
}

void SignalThreadExample()
{
	struct sigaction previous_alarm_action;
	struct sigaction previous_interrupt_action;
	if (!install_handler(SIGALRM, &previous_alarm_action))
		return;
	if (!install_handler(SIGINT, &previous_interrupt_action))
	{
		(void)::sigaction(SIGALRM, &previous_alarm_action, nullptr);
		return;
	}

	g_alarm_count = 0;
	g_stop_requested = 0;

	std::cout << "【进程与线程信号示例】\n";
	std::cout << "PID=" << ::getpid()
		<< ", main pthread_self()=" << current_thread_id() << '\n';
	std::cout << "每 " << kAlarmIntervalSeconds << " 秒触发一次 SIGALRM，共 "
		<< kMaximumAlarmCount << " 次；按 Ctrl+C 可发送 SIGINT 提前结束。\n";
	std::cout << "提示：处理函数仅设置标志并使用 write() 输出；线程 ID 在安全的主流程中输出。\n";

	::alarm(kAlarmIntervalSeconds);
	sig_atomic_t handled_alarm_count = 0;
	while (g_stop_requested == 0 && handled_alarm_count < kMaximumAlarmCount)
	{
		if (::pause() == -1 && errno != EINTR)
		{
			std::perror("pause failed");
			break;
		}

		while (handled_alarm_count < g_alarm_count &&
			handled_alarm_count < kMaximumAlarmCount)
		{
			++handled_alarm_count;
			std::cout << "main: handled SIGALRM #" << handled_alarm_count
				<< ", PID=" << ::getpid()
				<< ", pthread_self()=" << current_thread_id() << '\n';

			if (handled_alarm_count < kMaximumAlarmCount && g_stop_requested == 0)
				::alarm(kAlarmIntervalSeconds);
		}
	}

	::alarm(0);
	if (g_stop_requested != 0)
	{
		std::cout << "main: SIGINT observed; example ended early.\n";
	}
	else if (handled_alarm_count == kMaximumAlarmCount)
	{
		std::cout << "main: all SIGALRM demonstrations completed.\n";
	}

	if (::sigaction(SIGALRM, &previous_alarm_action, nullptr) == -1)
		std::perror("restore SIGALRM handler failed");
	if (::sigaction(SIGINT, &previous_interrupt_action, nullptr) == -1)
		std::perror("restore SIGINT handler failed");
}
