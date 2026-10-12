#include "SessionExample.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void set_sid()
{
    // setsid() 不能由进程组组长调用；fork 后的子进程通常不是进程组组长。
    const pid_t child_pid = ::fork();
    if (child_pid == -1)
    {
        std::perror("fork failed");
        return;
    }

    if (child_pid > 0)
    {
        // 父进程等待子进程，避免子进程结束后暂时成为僵尸进程。
        int status = 0;
        if (::waitpid(child_pid, &status, 0) == -1)
        {
            std::perror("waitpid failed");
        }
        else if (WIFEXITED(status))
        {
            std::cout << "parent: child " << child_pid
                      << " exited with code " << WEXITSTATUS(status) << '\n';
        }
        else if (WIFSIGNALED(status))
        {
            std::cout << "parent: child " << child_pid
                      << " was terminated by signal " << WTERMSIG(status) << '\n';
        }
        return;
    }

    const auto print_session_info = [](const char* stage)
    {
        std::cout << stage << ": pid=" << ::getpid() << ", ppid=" << ::getppid()
                  << ", pgid=" << ::getpgrp() << ", sid=" << ::getsid(0) << std::endl;
    };

    print_session_info("child before setsid");
    // 成功后，子进程成为新会话首进程和新进程组组长，SID 与 PGID 都等于自己的 PID。
    if (::setsid() == -1)
    {
        std::perror("setsid failed");
        _exit(EXIT_FAILURE);
    }
    print_session_info("child after setsid");

    // setsid() 只创建新会话，不会自动创建守护进程，也不会关闭已有的标准输出。
    _exit(EXIT_SUCCESS);
}
