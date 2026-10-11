#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <sys/wait.h>
#include <unistd.h>
#include<fcntl.h>
#include<unistd.h>
#include<cstdio>
void str_to_num_fun()
{
    long long_value = 0;
    int int_value = 0;

    // 验证当前 Linux 目标平台的数据模型，不要假设各类型的字节数。
    std::cout << "sizeof(long): " << sizeof(long_value)
              << ", sizeof(int): " << sizeof(int_value)
              << ", sizeof(long long): " << sizeof(long long)
              << ", sizeof(long double): " << sizeof(long double) << '\n';

    // atoi、atol、atoll 和 atof 仅演示基础转换；它们无法区分错误输入和合法的 0。
    std::cout << "atoi(\"123456\"): " << std::atoi("123456") << '\n';
    std::cout << "atol(\"234567\"): " << std::atol("234567") << '\n';
    std::cout << "atoll(\"1234567890123\"): " << std::atoll("1234567890123") << '\n';
    std::cout << "atof(\"1.234567\"): " << std::atof("1.234567") << '\n';

    // strtol 通过 end_ptr 返回未转换的后缀，并通过 errno 报告数值范围错误。
    const char* long_text = "-987654321";
    char* end_ptr = nullptr;
    errno = 0;
    const long long_result = std::strtol(long_text, &end_ptr, 10);
    std::cout << "strtol(\"" << long_text << "\", base 10): " << long_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno
              << (errno == ERANGE ? " (range error)" : "") << '\n';

    // abc 后缀未被转换，验证 strtoll 会在遇到第一个非数字字符时停止。
    const char* long_long_text = "-987654321000abc123456";
    errno = 0;
    const long long long_long_result = std::strtoll(long_long_text, &end_ptr, 10);
    std::cout << "strtoll(\"" << long_long_text << "\", base 10): " << long_long_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno
              << (errno == ERANGE ? " (range error)" : "") << '\n';

    // base 为 0 时会根据前缀自动识别进制：0x 表示十六进制，前导 0 表示八进制。
    const char* hex_text = "0x7f";
    errno = 0;
    const long hex_result = std::strtol(hex_text, &end_ptr, 0);
    std::cout << "strtol(\"" << hex_text << "\", base 0): " << hex_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    const char* octal_text = "0755";
    errno = 0;
    const long octal_result = std::strtol(octal_text, &end_ptr, 0);
    std::cout << "strtol(\"" << octal_text << "\", base 0): " << octal_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    // 指定 base 为 2 时，字符串按二进制解释；strtol 会自动忽略开头的空白和正负号。
    const char* binary_text = "  +101101";
    errno = 0;
    const long binary_result = std::strtol(binary_text, &end_ptr, 2);
    std::cout << "strtol(\"" << binary_text << "\", base 2): " << binary_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    // end_ptr 与输入首地址相同，表示输入中没有任何可转换的数字。
    const char* invalid_integer_text = "abc";
    errno = 0;
    const long invalid_integer_result = std::strtol(invalid_integer_text, &end_ptr, 10);
    std::cout << "strtol(\"" << invalid_integer_text << "\", base 10): " << invalid_integer_result
              << ", no digits: " << (end_ptr == invalid_integer_text)
              << ", errno: " << errno << '\n';

    // 超出 long long 范围时，返回边界值并设置 errno 为 ERANGE。
    const char* overflow_text = "9223372036854775808";
    errno = 0;
    const long long overflow_result = std::strtoll(overflow_text, &end_ptr, 10);
    std::cout << "strtoll(\"" << overflow_text << "\", base 10): " << overflow_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno
              << (errno == ERANGE ? " (range error)" : "") << '\n';

    // 使用同一文本分别验证转换为 float、double 和 long double 的精度差异。
    const char* floating_text = "1.234567890123";
    errno = 0;
    const float float_result = std::strtof(floating_text, &end_ptr);
    std::cout << std::setprecision(16)
              << "strtof(\"" << floating_text << "\"): " << float_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    errno = 0;
    const double double_result = std::strtod(floating_text, &end_ptr);
    std::cout << "strtod(\"" << floating_text << "\"): " << double_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    errno = 0;
    const long double long_double_result = std::strtold(floating_text, &end_ptr);
    std::cout << "strtold(\"" << floating_text << "\"): " << long_double_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << '\n';

    // strtod 支持科学计数法；ms 后缀留在 remaining 中，便于继续解析单位。
    const char* scientific_text = "6.02e23ms";
    errno = 0;
    const double scientific_result = std::strtod(scientific_text, &end_ptr);
    std::cout << std::scientific << "strtod(\"" << scientific_text << "\"): " << scientific_result
              << ", remaining: \"" << end_ptr << "\", errno: " << errno << std::defaultfloat << '\n';

    // C++ 标准库的 stoi/stod 以异常报告错误，position 返回成功转换的字符数量。
    try
    {
        std::size_t position = 0;
        const int cpp_integer_result = std::stoi("0x2aXYZ", &position, 0);
        std::cout << "stoi(\"0x2aXYZ\", base 0): " << cpp_integer_result
                  << ", parsed length: " << position << ", remaining: \"XYZ\"\n";

        position = 0;
        const double cpp_double_result = std::stod("3.14159rad", &position);
        std::cout << "stod(\"3.14159rad\"): " << cpp_double_result
                  << ", parsed length: " << position << ", remaining: \"rad\"\n";

        // std::stoll 遇到无法表示的大整数会抛出 std::out_of_range。
        static_cast<void>(std::stoll("999999999999999999999999"));
    }
    catch (const std::exception& error)
    {
        std::cout << "C++ conversion exception: " << error.what() << '\n';
    }
}

void num_to_str_fun()
{
    // 函数名沿用学习示例；ecvt、fcvt、gcvt 的实际功能是将浮点数转换为字符串。
    int decimal_point = 0;
    int sign = 0;

    // ecvt 的第二个参数是有效数字位数；decimal_point 表示小数点应插入的位置。
    std::cout << "ecvt(0.123456789, 10): "
              << ::ecvt(0.123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 小于 1 的正数会得到负的 decimal_point，表示小数点在首个数字的左侧。
    std::cout << "ecvt(0.0123456789, 10): "
              << ::ecvt(0.0123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 负数的 sign 为 1；返回字符串本身不包含负号。
    std::cout << "ecvt(-123.456789, 10): "
              << ::ecvt(-123.456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 整数的 decimal_point 等于整数部分的位数。
    std::cout << "ecvt(100.0, 4): "
              << ::ecvt(100.0, 4, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 0 的 decimal_point 为 1；这有助于将结果字符串还原成 0.0000 等形式。
    std::cout << "ecvt(0.0, 5): "
              << ::ecvt(0.0, 5, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 有效数字不足以完整表示时，ecvt 会按当前 C 运行库的规则进行舍入。
    std::cout << "ecvt(9.9999, 3): "
              << ::ecvt(9.9999, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    std::cout << "----------------------------------------\n";

    // fcvt 的第二个参数是小数点后的位数，返回结果不包含小数点。
    std::cout << "fcvt(0.123456789, 10): "
              << ::fcvt(0.123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 验证 fcvt 会保留指定的小数位，即使数值非常接近 0。
    std::cout << "fcvt(0.0123456789, 10): "
              << ::fcvt(0.0123456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 验证 fcvt 对负数同样通过 sign 返回符号信息。
    std::cout << "fcvt(-123.456789, 10): "
              << ::fcvt(-123.456789, 10, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 整数使用 fcvt 时仍会按指定的小数位补零。
    std::cout << "fcvt(100.0, 3): "
              << ::fcvt(100.0, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 指定两位小数会触发舍入，12.34567 的转换结果可用来观察这一行为。
    std::cout << "fcvt(12.34567, 2): "
              << ::fcvt(12.34567, 2, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    // 负零仍可能通过 sign 保留符号信息，便于处理科学计算中的符号位。
    std::cout << "fcvt(-0.0, 3): "
              << ::fcvt(-0.0, 3, &decimal_point, &sign)
              << ", decimal_point: " << decimal_point << ", sign: " << sign << '\n';

    std::cout << "----------------------------------------\n";

    char buffer[128] = {};
    // gcvt 将包含小数点和负号的可读字符串写入调用者提供的缓冲区。
    std::cout << "gcvt(0.123456789, 10): " << ::gcvt(0.123456789, 10, buffer) << '\n';
    std::cout << "gcvt(0.0123456789, 10): " << ::gcvt(0.0123456789, 10, buffer) << '\n';
    std::cout << "gcvt(-123.456789, 10): " << ::gcvt(-123.456789, 10, buffer) << '\n';
    std::cout << "gcvt(100.0, 10): " << ::gcvt(100.0, 10, buffer) << '\n';
    // 大数和极小数通常会使用科学计数法，以避免生成过长的字符串。
    std::cout << "gcvt(123456789.0, 4): " << ::gcvt(123456789.0, 4, buffer) << '\n';
    std::cout << "gcvt(0.000000012345, 6): " << ::gcvt(0.000000012345, 6, buffer) << '\n';

    // ecvt/fcvt 返回由 C 库维护的静态缓冲区，下一次调用会覆盖上一次结果；应立即使用或复制。
    // gcvt 使用调用者提供的 buffer，buffer 必须足够大，且下一次 gcvt 调用会覆盖该 buffer 的内容。
    std::cout << "----------------------------------------\n";
    std::cout << "std::to_string(-123.456789): " << std::to_string(-123.456789) << '\n';

    // ostringstream 是 C++ 中控制小数位数的常用方式，适合替代旧式 fcvt/gcvt。
    std::ostringstream formatted_number;
    formatted_number << std::fixed << std::setprecision(3) << 12.34567;
    std::cout << "ostringstream fixed(3) for 12.34567: " << formatted_number.str() << '\n';
}

void stdarg_test(const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);

    // vprintf 会推进 va_list 的读取位置，因此为终端输出创建独立副本。
    va_list print_arguments;
    va_copy(print_arguments, arguments);
    std::vprintf(format, print_arguments);
    va_end(print_arguments);

    char buffer[4096] = {};
    // 为缓冲区格式化再次创建副本，并使用带长度限制的 vsnprintf 防止越界。
    va_list buffer_arguments;
    va_copy(buffer_arguments, arguments);
    const int written = std::vsnprintf(buffer, sizeof(buffer), format, buffer_arguments);
    va_end(buffer_arguments);
    va_end(arguments);

    if (written < 0)
    {
        std::perror("vsnprintf failed");
        return;
    }
    if (static_cast<std::size_t>(written) >= sizeof(buffer))
    {
        std::cout << "test buffer output was truncated\n";
        return;
    }

    // 输出缓冲区内容，验证 vprintf 和 vsnprintf 使用的是同一组可变参数。
    std::cout << std::endl << "test buffer: " << buffer;
}

void print_fun()
{
    const int integer_value = 1;
    const unsigned int unsigned_value = 2U;
    const long long_value = 3L;
    const long long long_long_value = 4LL;
    const double floating_value = 0.00001;

    std::printf("hello world\n");

    // 每个占位符都与参数类型对应，避免可变参数类型不匹配造成未定义行为。
    std::printf("%%d=%d, %%u=%u, %%ld=%ld, %%lld=%lld\n",
                integer_value, unsigned_value, long_value, long_long_value);
    // 同一 double 使用三种格式输出：普通小数、科学计数法和紧凑格式。
    std::printf("%%f=%f, %%e=%e, %%g=%g\n",
                floating_value, floating_value, floating_value);
    std::printf("%%s=%s\n", "hello world");

    char buffer[128] = {};
    // snprintf 会限制写入长度，并返回“理论上需要写入”的字符数（不含结尾空字符）。
    const int written = std::snprintf(buffer, sizeof(buffer), "%s %d %.6f\n",
                                      "yidaoyun", 100, 100.00001);
    if (written < 0)
    {
        std::perror("snprintf failed");
        return;
    }
    if (static_cast<std::size_t>(written) >= sizeof(buffer))
    {
        std::cout << "snprintf output was truncated\n";
    }
    else
    {
        std::cout << "snprintf wrote " << written << " characters: " << buffer;
    }

    // 文件写入不会显示在终端；test.txt 位于远端 Linux 程序的当前工作目录。
    std::FILE* file = std::fopen("test.txt", "w+");
    if (file == nullptr)
    {
        std::perror("fopen test.txt failed");
        return;
    }

    if (std::fprintf(file, "%s %d %.6f\n", "yidaoyun", 100, 100.00001) < 0)
    {
        std::perror("fprintf test.txt failed");
    }
    if (std::fclose(file) != 0)
    {
        std::perror("fclose test.txt failed");
    }
    stdarg_test("%%d=%d, %%u=%u, %%ld=%ld, %%lld=%lld\n",
        integer_value, unsigned_value, long_value, long_long_value);
}

void scanf_fun()
{
    char c;
    int d;
    double f;
    //输入是空格可有可无（整数小数之间必须要有），如果采用了其他的分隔符则输入必须要有
    scanf("%c%d%f", &c, &d, &f);
    stdarg_test("inputchar = %c inputint = %d inputdouble = %lf", c, d, f);
}

void get_uid()
{
    const auto print_ids = []()
    {
        std::cout << "uid=" << getuid() << ", euid=" << geteuid() << '\n';
        std::cout << "gid=" << getgid() << ", egid=" << getegid() << '\n';
    };

    std::cout << "before changing identity:\n";
    print_ids();

    // 请以普通用户直接运行、且移除文件的 Setuid/Setgid 位，验证低权限无法切换到 root。
    errno = 0;
    if (setuid(0) == -1)
    {
        std::perror("setuid(0)");
    }
    else
    {
        std::cout << "setuid(0) succeeded\n";
    }

    errno = 0;
    if (setgid(0) == -1)
    {
        std::perror("setgid(0)");
    }
    else
    {
        std::cout << "setgid(0) succeeded\n";
    }

    std::cout << "after changing identity:\n";
    print_ids();
}

void set_sid()
{
    // setsid() 不能由进程组组长调用；fork 后的子进程通常不是进程组组长。
    const pid_t child_pid = fork();
    if (child_pid == -1)
    {
        std::perror("fork failed");
        return;
    }

    if (child_pid > 0)
    {
        // 父进程等待子进程，避免子进程结束后暂时成为僵尸进程。
        int status = 0;
        if (waitpid(child_pid, &status, 0) == -1)
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
        std::cout << stage
                  << ": pid=" << getpid()
                  << ", ppid=" << getppid()
                  << ", pgid=" << getpgrp()
                  << ", sid=" << getsid(0) << std::endl;
    };

    print_session_info("child before setsid");

    // 成功后，子进程成为新会话首进程和新进程组组长，SID 与 PGID 都等于自己的 PID。
    if (setsid() == -1)
    {
        std::perror("setsid failed");
        _exit(EXIT_FAILURE);
    }

    print_session_info("child after setsid");

    // setsid() 只创建新会话，不会自动创建守护进程，也不会关闭已有的标准输出。
    _exit(EXIT_SUCCESS);
}

void IO_fun()
{
    const char* file_name = "/home/lisen/io_demo.txt";
    const char* text = "open/write/lseek/read/close demonstration\n";

    // O_RDWR：可读可写；O_CREAT：文件不存在时创建；O_TRUNC：清空原有内容，便于每次观察一致结果。
    // 0644 是新建文件的期望权限（rw-r--r--），实际权限还会受到进程 umask 的影响。
    const int fd = ::open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644); //权限是8进制的，必须要加上一个0
    if (fd == -1)
    {
        std::perror("open io_demo.txt failed");
        return;
    }
    std::cout << "open succeeded, fd=" << fd << '\n';

    // write() 可能只写入部分数据，循环写入才能保证完整写出；EINTR 表示被信号中断，可重试。
    const auto write_all = [fd](const char* data, std::size_t size)
    {
        std::size_t total_written = 0;
        while (total_written < size)
        {
            //::write标识调用Linux全局命名空间中的系统调用，返回的written为本次write写入的字节数目
            const ssize_t written = ::write(fd, data + total_written, size - total_written);
            if (written > 0)
            {
                total_written += static_cast<std::size_t>(written);
                continue;
            }
            if (written == -1 && errno == EINTR)
            {
                continue;
            }
            return false;
        }
        return true;
    };

    const std::size_t text_size = std::strlen(text);
    if (!write_all(text, text_size))
    {
        std::perror("write io_demo.txt failed");
        ::close(fd);
        return;
    }
    std::cout << "write succeeded, byte count=" << text_size << '\n';

    // write() 返回成功只表示数据已交给内核页缓存，不代表已写入底层存储设备。
    // fsync() 请求内核将该文件的数据及必要元数据同步到存储设备；发生断电等故障时仍取决于设备和文件系统的保障能力。
    if (::fsync(fd) == -1)
    {
        std::perror("fsync io_demo.txt failed");
        ::close(fd);
        return;
    }
    std::cout << "fsync succeeded, kernel requested data synchronization\n";

    // 写入后文件偏移量位于文件末尾；lseek 将其移动回开头，随后 read() 才能读到刚写入的内容。
    // 0 和 SEEK_SET：前者指的的偏移量，后者指的是移动的目标位置，SEEK_SET指的是移动到文件开头，并且向后偏移0个字节的位置
    if (::lseek(fd, 0, SEEK_SET) == static_cast<off_t>(-1))
    {
        std::perror("lseek io_demo.txt failed");
        ::close(fd);
        return;
    }
    std::cout << "lseek succeeded, current offset moved to file beginning\n";

    std::string content;
    //每次最多读取16个字节的缓冲区
    char buffer[16] = {};
    while (true)
    {
        const ssize_t read_count = ::read(fd, buffer, sizeof(buffer));
        if (read_count > 0)
        {
            //每次向content中追加本次读取长度的内容
            content.append(buffer, static_cast<std::size_t>(read_count));
            continue;
        }
        if (read_count == 0)
        {
            // read() 返回 0 表示已到达文件末尾（EOF）。
            break;
        }
        //errno是<cerrno>文件中包含的宏，会根据当前线程返回对应的错误码存储位置
        //当发生错误之后可以输出errno来查看错误码，输出std::strerror(errno)来查看错误说明
        //std::cerr << "read failed: "<< errno << ", "<< std::strerror(errno) << '\n';
        if (errno == EINTR)
        {
            continue;
        }
        std::perror("read io_demo.txt failed");
        ::close(fd);
        return;
    }
    std::cout << "read succeeded, content: " << content;

    // close() 释放文件描述符；打开成功的 fd 都应在不再使用时关闭。
    if (::close(fd) == -1)
    {
        std::perror("close io_demo.txt failed");
    }
}

void process_control_fun()
{
    // exec*() 成功后不会返回：它会用新程序替换当前子进程的代码、数据和栈。
    // 因此每个示例都先 fork()，避免替换正在运行的主程序。
    enum class ExecExample
    {
        Execl,
        Execle,
        Execv,
        Execve,
        Execvp
    };

    const auto run_example = [](const char* api_name, ExecExample example)
    {
        std::cout << "\n--- " << api_name << " example ---\n" << std::flush;

        const pid_t child_pid = ::fork();
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
                // l 表示参数以列表（list）形式传入；最后一个 nullptr 是参数列表结束标志。
                ::execl("/bin/echo", "echo", "message from execl", static_cast<char*>(nullptr));
                break;

            case ExecExample::Execle:
            {
                // execle 也使用参数列表，但最后额外传入子进程的新环境变量数组。
                char environment_variable[] = "EXEC_DEMO=execle";
                char* environment[] = {environment_variable, nullptr};
                ::execle("/usr/bin/env", "env", static_cast<char*>(nullptr), environment);
                break;
            }

            case ExecExample::Execv:
            {
                // v 表示参数通过 argv 数组传入；第一个元素按约定是程序名。
                char program_name[] = "echo";
                char message[] = "message from execv";
                char* arguments[] = {program_name, message, nullptr};
                ::execv("/bin/echo", arguments);
                break;
            }

            case ExecExample::Execve:
            {
                // execve 同时接收 argv 和 envp；它不会沿用父进程环境，而是使用此处提供的环境。
                char program_name[] = "env";
                char environment_variable[] = "EXEC_DEMO=execve";
                char* arguments[] = {program_name, nullptr};
                char* environment[] = {environment_variable, nullptr};
                ::execve("/usr/bin/env", arguments, environment);
                break;
            }

            case ExecExample::Execvp:
            {
                // p 表示在 PATH 环境变量指定的目录中查找程序，因此这里不必写 /bin/echo。
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

        // 父进程继续运行，并用 waitpid() 回收子进程，避免子进程结束后成为僵尸进程。
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

    // 依次观察五种 exec 函数的参数传递、PATH 查找和环境变量处理方式。
    run_example("execl", ExecExample::Execl);
    run_example("execle", ExecExample::Execle);
    run_example("execv", ExecExample::Execv);
    run_example("execve", ExecExample::Execve);
    run_example("execvp", ExecExample::Execvp);
}

int main()
{
    //num_to_str_fun();
    //str_to_num_fun();
    //print_fun();
    //scanf_fun();
    //get_uid();
    //set_sid();
    IO_fun();
    //process_control_fun();
    return 0;
}
