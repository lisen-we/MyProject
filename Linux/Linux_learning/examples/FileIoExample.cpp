#include "FileIoExample.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <fcntl.h>
#include <unistd.h>

void IO_fun()
{
    const char* file_name = "/home/lisen/io_demo.txt";
    const char* text = "open/write/lseek/read/close demonstration\n";

    // O_RDWR：可读可写；O_CREAT：文件不存在时创建；O_TRUNC：清空原有内容，便于每次观察一致结果。
    // 0644 是新建文件的期望权限（rw-r--r--），实际权限还会受到进程 umask 的影响。
    const int fd = ::open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644); // 权限是八进制，必须以前导 0 表示。
    if (fd == -1)
    {
        // perror 的效果大致等于“提示文字 + 当前 errno 对应的错误说明”。
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
            // ::write 调用 Linux 全局命名空间中的系统调用；written 是本次实际写入的字节数。
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

    // write() 成功只表示数据已交给内核页缓存；fsync() 请求同步该文件的数据及必要元数据。
    if (::fsync(fd) == -1)
    {
        std::perror("fsync io_demo.txt failed");
        ::close(fd);
        return;
    }
    std::cout << "fsync succeeded, kernel requested data synchronization\n";

    // 写入后偏移量在文件末尾；offset=0、SEEK_SET 表示移动到文件开头后再读取。
    if (::lseek(fd, 0, SEEK_SET) == static_cast<off_t>(-1))
    {
        std::perror("lseek io_demo.txt failed");
        ::close(fd);
        return;
    }
    std::cout << "lseek succeeded, current offset moved to file beginning\n";

    std::string content;
    char buffer[16] = {}; // 每次最多读取 16 个字节。
    while (true)
    {
        const ssize_t read_count = ::read(fd, buffer, sizeof(buffer));
        if (read_count > 0)
        {
            content.append(buffer, static_cast<std::size_t>(read_count));
            continue;
        }
        if (read_count == 0)
        {
            // read() 返回 0 表示已到达文件末尾（EOF）。
            break;
        }
        // errno 是 <cerrno> 提供的线程局部错误码访问宏；仅在 read() 返回 -1 后检查它。
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
