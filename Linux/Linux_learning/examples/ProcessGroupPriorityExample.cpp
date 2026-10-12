#include "ProcessGroupPriorityExample.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

void process_group_priority_fun()
{
    std::cout << "\n--- process ID / process group example ---\n";

    // getpid() 返回当前 PID；getppid() 返回创建当前进程的父进程 PID。
    const pid_t current_pid = ::getpid();
    std::cout << "getpid(): " << current_pid << ", getppid(): " << ::getppid() << '\n';

    // getpgid(pid)：查询指定 PID 的进程组；传入 current_pid（或 0）都表示当前进程。
    const pid_t group_id_by_pid = ::getpgid(current_pid);
    if (group_id_by_pid == -1)
    {
        std::perror("getpgid(current_pid) failed");
        return;
    }
    // getpgrp() 无参数，等价于 getpgid(0)。
    std::cout << "getpgid(current_pid): " << group_id_by_pid
              << ", getpgrp(): " << ::getpgrp() << '\n';

    const auto print_current_priority = [](const char* stage)
    {
        // getpriority() 成功时允许返回 -1（nice 值 -1 合法），所以必须在调用前清零 errno。
        errno = 0;
        // which=PRIO_PROCESS：按进程查询；who=0：当前进程。
        const int priority = ::getpriority(PRIO_PROCESS, 0);
        if (priority == -1 && errno != 0)
        {
            std::perror("getpriority(PRIO_PROCESS, 0) failed");
            return false;
        }
        // nice 值范围通常为 -20 到 19；值越小，调度优先级越高。
        std::cout << stage << ": nice value=" << priority << '\n';
        return true;
    };

    std::cout << "\n--- getpriority example ---\n";
    if (!print_current_priority("parent"))
    {
        return;
    }

    const auto wait_for_child = [](pid_t child_pid, const char* example_name)
    {
        int status = 0;
        if (::waitpid(child_pid, &status, 0) == -1)
        {
            std::perror("waitpid failed");
        }
        else if (WIFEXITED(status))
        {
            std::cout << "parent: " << example_name << " child exited with code "
                      << WEXITSTATUS(status) << '\n';
        }
        else if (WIFSIGNALED(status))
        {
            std::cout << "parent: " << example_name << " child was terminated by signal "
                      << WTERMSIG(status) << '\n';
        }
    };

    const auto run_group_change_example = [&wait_for_child](bool use_setpgrp)
    {
        const char* api_name = use_setpgrp ? "setpgrp" : "setpgid";
        std::cout << "\n--- " << api_name << " example (child only) ---\n" << std::flush;

        const pid_t child_pid = ::fork();
        if (child_pid == -1)
        {
            std::perror("fork failed");
            return;
        }
        if (child_pid == 0)
        {
            std::cout << "child before change: pid=" << ::getpid()
                      << ", pgid=" << ::getpgrp() << '\n';
            // setpgid(pid, pgid)：pid=0 表示当前进程；pgid=0 表示用当前 PID 作为新进程组 ID。
            // Linux 的无参 setpgrp() 等价于 setpgid(0, 0)，生产代码优先使用可移植性更好的 setpgid()。
            const int result = use_setpgrp ? ::setpgrp() : ::setpgid(0, 0);
            if (result == -1)
            {
                std::perror(api_name);
                _exit(EXIT_FAILURE);
            }
            std::cout << "child after " << api_name << ": pid=" << ::getpid()
                      << ", pgid=" << ::getpgrp() << '\n' << std::flush;
            _exit(EXIT_SUCCESS);
        }
        wait_for_child(child_pid, api_name);
    };

    // 子进程中改变进程组，不影响调用程序的终端作业控制。
    run_group_change_example(false);
    run_group_change_example(true);

    std::cout << "\n--- setpriority / nice example (child only) ---\n" << std::flush;
    const pid_t priority_child_pid = ::fork();
    if (priority_child_pid == -1)
    {
        std::perror("fork failed");
        return;
    }
    if (priority_child_pid == 0)
    {
        if (!print_current_priority("child before changing priority"))
        {
            _exit(EXIT_FAILURE);
        }

        errno = 0;
        const int current_priority = ::getpriority(PRIO_PROCESS, 0);
        if (current_priority == -1 && errno != 0)
        {
            std::perror("getpriority before setpriority failed");
            _exit(EXIT_FAILURE);
        }

        // setpriority(which, who, prio)：PRIO_PROCESS 操作进程，who=0 指当前子进程。
        // 将 nice 值增大 1 会降低优先级，普通用户通常有权限执行；最大值为 19。
        const int requested_priority = current_priority < 19 ? current_priority + 1 : 19;
        if (::setpriority(PRIO_PROCESS, 0, requested_priority) == -1)
        {
            std::perror("setpriority(PRIO_PROCESS, 0, requested_priority) failed");
            _exit(EXIT_FAILURE);
        }
        print_current_priority("child after setpriority");

        // nice(inc)：在当前 nice 值上加 inc；成功返回 -1 仍可能合法，故同样检查 errno。
        errno = 0;
        const int nice_result = ::nice(1);
        if (nice_result == -1 && errno != 0)
        {
            std::perror("nice(1) failed");
            _exit(EXIT_FAILURE);
        }
        std::cout << "nice(1) returned: " << nice_result << '\n';
        print_current_priority("child after nice(1)");
        std::cout << std::flush;
        _exit(EXIT_SUCCESS);
    }
    wait_for_child(priority_child_pid, "setpriority / nice");
}
