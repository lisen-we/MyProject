#include "ProcessControlExample.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <setjmp.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
// atexit() 只接受无参数回调，因此回调必须是独立函数。
void process_atexit_handler()
{
    std::puts("atexit callback: executed during normal exit");
}

// on_exit() 会将 exit(status) 的状态码和注册时提供的 arg 原样传回。
void process_on_exit_handler(int status, void* arg)
{
    const char* label = static_cast<const char*>(arg);
    std::printf("on_exit callback: status=%d, arg=%s\n", status, label);
}
} // namespace

void process_control_fun()
{
    // exec*() 成功后不返回：它会以目标程序替换当前子进程的代码、数据和栈。
    enum class ExecExample
    {
        // l（list）：参数逐个传入，最后以 nullptr 结束。
        Execl,
        // l + e（environment）：参数列表末尾额外传入 envp 环境变量数组。
        Execle,
        // v（vector）：通过 argv 字符串指针数组传入参数，继承当前环境。
        Execv,
        // v + e：显式传入 argv 和 envp，不继承当前环境。
        Execve,
        // v + p（PATH）：通过 argv 传参，并依据 PATH 查找程序。
        Execvp
    };

    const auto run_example = [](const char* api_name, ExecExample example)
    {
        std::cout << "\n--- " << api_name << " example ---\n" << std::flush;
        const pid_t child_pid = ::fork();
        // fork 返回值：大于 0 时在父进程中，值是子进程 PID；等于 0 时在子进程中；-1 表示失败。
        if (child_pid == -1)
        {
            std::perror("fork failed");
            return;
        }

        if (child_pid == 0)
        {
            switch (example)
            {
            case ExecExample::Execl:
                ::execl("/bin/echo", "echo", "message from execl", static_cast<char*>(nullptr));
                break;
            case ExecExample::Execle:
            {
                char environment_variable[] = "EXEC_DEMO=execle";
                char* environment[] = {environment_variable, nullptr};
                ::execle("/usr/bin/env", "env", static_cast<char*>(nullptr), environment);
                break;
            }
            case ExecExample::Execv:
            {
                char program_name[] = "echo";
                char message[] = "message from execv";
                char* arguments[] = {program_name, message, nullptr};
                ::execv("/bin/echo", arguments);
                break;
            }
            case ExecExample::Execve:
            {
                char program_name[] = "env";
                char environment_variable[] = "EXEC_DEMO=execve";
                char* arguments[] = {program_name, nullptr};
                char* environment[] = {environment_variable, nullptr};
                ::execve("/usr/bin/env", arguments, environment);
                break;
            }
            case ExecExample::Execvp:
            {
                char program_name[] = "echo";
                char message[] = "message from execvp (found through PATH)";
                char* arguments[] = {program_name, message, nullptr};
                ::execvp(program_name, arguments);
                break;
            }
            }

            // 只有 exec*() 失败时才会继续执行到这里；127 是常用的“命令无法执行”退出码。
            std::perror(api_name);
            _exit(127);
        }

        // waitpid(pid, &status, 0)：等待指定 PID 的子进程；status 需由宏解码。
        int status = 0;
        if (::waitpid(child_pid, &status, 0) == -1)
        {
            std::perror("waitpid failed");
        }
        else if (WIFEXITED(status))
        {
            std::cout << "parent: " << api_name << " child exited with code "
                      << WEXITSTATUS(status) << '\n';
        }
        else if (WIFSIGNALED(status))
        {
            std::cout << "parent: " << api_name << " child was terminated by signal "
                      << WTERMSIG(status) << '\n';
        }
    };

    run_example("execl", ExecExample::Execl);
    run_example("execle", ExecExample::Execle);
    run_example("execv", ExecExample::Execv);
    run_example("execve", ExecExample::Execve);
    run_example("execvp", ExecExample::Execvp);

    const auto run_termination_example = [](bool use_exit)
    {
        const char* api_name = use_exit ? "exit" : "_exit";
        std::cout << "\n--- " << api_name << " / atexit / on_exit example ---\n" << std::flush;
        const pid_t child_pid = ::fork();
        if (child_pid == -1)
        {
            std::perror("fork failed");
            return;
        }
        if (child_pid == 0)
        {
            // atexit(void (*)()) 注册正常 exit() 时调用的无参回调。
            if (std::atexit(process_atexit_handler) != 0)
            {
                std::fputs("atexit registration failed\n", stderr);
                _exit(127);
            }

            // on_exit(callback, arg) 是 Linux/glibc 扩展；callback 接收退出状态和 arg。
            char callback_argument[] = "registered in child";
            if (::on_exit(process_on_exit_handler, callback_argument) != 0)
            {
                std::fputs("on_exit registration failed\n", stderr);
                _exit(127);
            }

            if (use_exit)
            {
                // exit(42) 执行已注册回调并刷新 C 标准 I/O 流。
                std::cout << "child: calling exit(42)\n" << std::flush;
                std::exit(42);
            }

            // _exit(43) 立即结束，不执行回调也不刷新 C 标准 I/O 缓冲区。
            std::cout << "child: calling _exit(43), callbacks will be skipped\n" << std::flush;
            _exit(43);
        }

        int status = 0;
        if (::waitpid(child_pid, &status, 0) == -1)
        {
            std::perror("waitpid failed");
        }
        else if (WIFEXITED(status))
        {
            std::cout << "parent: " << api_name << " child exited with code "
                      << WEXITSTATUS(status) << '\n';
        }
    };

    // 在子进程中演示，避免 exit/_exit 结束主程序。
    run_termination_example(true);
    run_termination_example(false);

    std::cout << "\n--- setjmp / longjmp example ---\n";
    jmp_buf jump_environment;
    // setjmp(environment) 首次返回 0；longjmp(environment, 7) 跳回后使其返回 7。
    if (::setjmp(jump_environment) == 0)
    {
        std::cout << "setjmp first return: 0; calling longjmp(environment, 7)\n";
        ::longjmp(jump_environment, 7);
    }
    else
    {
        std::cout << "longjmp completed: setjmp returned 7\n";
    }

    std::cout << "\n--- sigsetjmp / siglongjmp example ---\n";
    sigjmp_buf signal_jump_environment;
    // save_mask=1 表示除执行环境外，还保存当前线程的信号屏蔽字。
    if (::sigsetjmp(signal_jump_environment, 1) == 0)
    {
        std::cout << "sigsetjmp first return: 0; calling siglongjmp(environment, 9)\n";
        ::siglongjmp(signal_jump_environment, 9);
    }
    else
    {
        std::cout << "siglongjmp completed: sigsetjmp returned 9\n";
    }

    // longjmp/siglongjmp 会跳过中间代码；不要跨越含非平凡析构函数的 C++ 自动对象。
    // 它们用于非局部跳转，不用于改变进程优先级。
}
