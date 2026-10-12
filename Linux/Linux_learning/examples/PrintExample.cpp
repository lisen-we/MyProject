#include "PrintExample.h"

#include "VariadicExample.h"

#include <cstdio>
#include <iostream>

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
    std::printf("%%f=%f, %%e=%e, %%g=%g\n", floating_value, floating_value, floating_value);
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
