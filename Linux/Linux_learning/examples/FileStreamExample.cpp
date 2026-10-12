#include "FileStreamExample.h"

#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

void file_stream_fun()
{
    const char* stdio_file_name = "/home/lisen/fopen_demo.txt";
    const char* descriptor_file_name = "/home/lisen/fdopen_demo.txt";

    std::cout << "\n--- fopen / fileno example ---\n";
    // fopen(filename, mode)：filename 是文件路径；"w+" 表示读写打开、文件不存在则创建、已有内容会被清空。
    std::FILE* stream = std::fopen(stdio_file_name, "w+");
    if (stream == nullptr)
    {
        std::perror("fopen fopen_demo.txt failed");
        return;
    }
    printf("%s(%d):%s\n", __FILE__, __LINE__, __FUNCTION__);
    const char* stdio_text = "text written through FILE*\n";
    if (std::fputs(stdio_text, stream) == EOF)
    {
        std::perror("fputs fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }

    // FILE* 使用用户态缓冲；fflush() 先把缓冲内容交给内核，便于后续通过 fd 观察文件状态。
    if (std::fflush(stream) != 0)
    {
        std::perror("fflush fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }

    // fileno(stream) 返回 FILE* 底层使用的文件描述符；返回 -1 表示获取失败。
    const int descriptor_from_stream = ::fileno(stream);
    if (descriptor_from_stream == -1)
    {
        std::perror("fileno failed");
        std::fclose(stream);
        return;
    }
    std::cout << "fopen returned FILE*, fileno(stream) returned fd="
              << descriptor_from_stream << '\n';

    // fseek(stream, 0, SEEK_SET) 将流位置移动到开头，再使用 fgets() 从 FILE* 读取刚写入的文本。
    if (std::fseek(stream, 0, SEEK_SET) != 0)
    {
        std::perror("fseek fopen_demo.txt failed");
        std::fclose(stream);
        return;
    }
    char buffer[128] = {};
    if (std::fgets(buffer, sizeof(buffer), stream) == nullptr)
    {
        if (std::ferror(stream) != 0)
        {
            std::perror("fgets fopen_demo.txt failed");
        }
        else
        {
            std::cout << "fgets reached EOF before reading text\n";
        }
        std::fclose(stream);
        return;
    }
    std::cout << "read through FILE*: " << buffer;

    // fclose() 会关闭 FILE*，同时也会关闭 fileno() 返回的底层文件描述符。
    if (std::fclose(stream) != 0)
    {
        std::perror("fclose fopen_demo.txt failed");
        return;
    }

    std::cout << "\n--- open / fdopen example ---\n";
    // open() 返回 fd；O_RDWR 与 fdopen() 的 "w+" 读写需求兼容，O_TRUNC 用于清空旧测试内容。
    const int descriptor = ::open(descriptor_file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (descriptor == -1)
    {
        std::perror("open fdopen_demo.txt failed");
        return;
    }

    // fdopen(fd, mode)：将已打开的 fd 包装为 FILE*；mode 必须与 fd 的访问方式兼容。
    std::FILE* descriptor_stream = ::fdopen(descriptor, "w+");
    if (descriptor_stream == nullptr)
    {
        std::perror("fdopen failed");
        ::close(descriptor);
        return;
    }

    const char* descriptor_text = "text written after fdopen\n";
    if (std::fputs(descriptor_text, descriptor_stream) == EOF || std::fflush(descriptor_stream) != 0)
    {
        std::perror("write through fdopen stream failed");
        std::fclose(descriptor_stream);
        return;
    }

    if (std::fseek(descriptor_stream, 0, SEEK_SET) != 0)
    {
        std::perror("fseek fdopen_demo.txt failed");
        std::fclose(descriptor_stream);
        return;
    }
    std::memset(buffer, 0, sizeof(buffer));
    if (std::fgets(buffer, sizeof(buffer), descriptor_stream) != nullptr)
    {
        std::cout << "read after fdopen: " << buffer;
    }
    else if (std::ferror(descriptor_stream) != 0)
    {
        std::perror("fgets fdopen_demo.txt failed");
    }
    else
    {
        std::cout << "fgets reached EOF before reading text\n";
    }

    // fdopen() 成功后，FILE* 接管 descriptor 的关闭责任；只能调用 fclose()，不能再额外 close(descriptor)。
    if (std::fclose(descriptor_stream) != 0)
    {
        std::perror("fclose fdopen_demo.txt failed");
    }
}
