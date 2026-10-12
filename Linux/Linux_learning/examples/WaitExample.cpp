#include "WaitExample.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void wait_fun()
{
    std::cout << "\n--- wait example ---\n" << std::flush;
    const pid_t child_pid = ::fork();
    if (child_pid == -1)
    {
        std::perror("fork failed");
        return;
    }

    if (child_pid == 0)
    {
        // 子进程用 24 作为结束状态；_exit() 避免执行父进程可能注册的退出回调。
        std::cout << "child: pid=" << ::getpid() << ", exiting with code 24\n" << std::flush;
        _exit(24);
    }

    int status = 0;
    // wait(status_ptr)：status_ptr 指向接收状态字的变量；nullptr 表示丢弃状态。
    // wait() 阻塞等待任意一个子进程结束，并返回实际被回收的子进程 PID。
    const pid_t reaped_pid = ::wait(&status);
    if (reaped_pid == -1)
    {
        std::perror("wait failed");
        return;
    }

    std::cout << "wait returned child pid=" << reaped_pid << '\n';
    // status 不是直接退出码，必须先用 WIFEXITED/WIFSIGNALED 等宏判断其含义。
    if (WIFEXITED(status))
    {
        std::cout << "child exited normally, exit code=" << WEXITSTATUS(status) << '\n';
    }
    else if (WIFSIGNALED(status))
    {
        std::cout << "child was terminated by signal=" << WTERMSIG(status) << '\n';
    }
}
