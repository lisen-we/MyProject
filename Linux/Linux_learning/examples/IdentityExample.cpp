#include "IdentityExample.h"

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <unistd.h>

void get_uid()
{
    const auto print_ids = []()
    {
        std::cout << "uid=" << ::getuid() << ", euid=" << ::geteuid() << '\n';
        std::cout << "gid=" << ::getgid() << ", egid=" << ::getegid() << '\n';
    };

    std::cout << "before changing identity:\n";
    print_ids();

    // 请以普通用户直接运行、且移除文件的 Setuid/Setgid 位，验证低权限无法切换到 root。
    errno = 0;
    if (::setuid(0) == -1)
    {
        std::perror("setuid(0)");
    }
    else
    {
        std::cout << "setuid(0) succeeded\n";
    }

    errno = 0;
    if (::setgid(0) == -1)
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
