#include "VariadicExample.h"

#include <cstdarg>
#include <cstdio>
#include <iostream>

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
