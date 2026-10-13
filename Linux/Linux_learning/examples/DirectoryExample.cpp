#include "DirectoryExample.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void directory_fun()
{
    // mkdtemp 会将 XXXXXX 替换为随机字符并创建目录，避免误操作固定路径或用户已有文件。
    char directory_template[] = "/tmp/linux_directory_demo_XXXXXX";
    char* const root_directory = ::mkdtemp(directory_template);
    if (root_directory == nullptr)
    {
        std::perror("mkdtemp failed");
        return;
    }

    const std::string root_path = root_directory;
    const std::string child_path = root_path + "/child";
    const std::string unlink_file_path = child_path + "/unlink_demo.txt";
    const std::string remove_file_path = child_path + "/remove_demo.txt";
    const std::string remove_directory_path = root_path + "/remove_empty_directory";

    // 仅清理本函数使用 mkdtemp/mkdir 创建的确定路径；忽略 ENOENT，避免覆盖原始错误信息。
    const auto cleanup = [&]()
    {
        ::unlink(unlink_file_path.c_str());
        ::remove(remove_file_path.c_str());
        ::rmdir(remove_directory_path.c_str());
        ::rmdir(child_path.c_str());
        ::rmdir(root_path.c_str());
    };

    std::cout << "directory demo root: " << root_path << '\n';

    // mkdir(pathname, mode)：pathname 是待创建路径；0750 是期望权限 rwxr-x---，实际结果还受 umask 影响。
    if (::mkdir(child_path.c_str(), 0750) == -1)
    {
        std::perror("mkdir child failed");
        cleanup();
        return;
    }
    std::cout << "mkdir created: " << child_path << '\n';

    // 创建由 unlink() 删除的普通文件。
    std::FILE* file = std::fopen(unlink_file_path.c_str(), "w");
    if (file == nullptr)
    {
        std::perror("fopen unlink_demo.txt failed");
        cleanup();
        return;
    }
    std::fputs("this file will be removed by unlink\n", file);
    if (std::fclose(file) != 0)
    {
        std::perror("fclose unlink_demo.txt failed");
        cleanup();
        return;
    }

    // chmod(path, mode)：修改已有文件或目录的权限。0640 对应 rw-r-----。
    if (::chmod(unlink_file_path.c_str(), 0640) == -1)
    {
        std::perror("chmod unlink_demo.txt failed");
        cleanup();
        return;
    }
    std::cout << "chmod set unlink_demo.txt to 0640\n";

    // chown(path, owner, group)：修改所有者和所属组。此处传入当前有效 UID/GID，不改变归属，安全演示参数含义。
    // 若改为其他用户或组，普通用户通常会得到 EPERM；应由 root 或具备 CAP_CHOWN 的进程执行。
    if (::chown(unlink_file_path.c_str(), ::geteuid(), ::getegid()) == -1)
    {
        std::cout << "chown to current effective UID/GID was not permitted: errno=" << errno << '\n';
    }
    else
    {
        std::cout << "chown used owner=" << ::geteuid() << ", group=" << ::getegid() << '\n';
    }

    std::cout << "\n--- opendir / readdir / closedir example ---\n";
    // opendir(name)：打开目录并返回 DIR*；DIR* 只能使用 readdir()/closedir() 等目录流函数操作。
    DIR* const directory = ::opendir(child_path.c_str());
    if (directory == nullptr)
    {
        std::perror("opendir child failed");
        cleanup();
        return;
    }

    while (true)
    {
        // readdir() 返回指向库内部缓冲区的 dirent*；下次 readdir() 或 closedir() 后不要再使用该指针。
        // 返回 nullptr 既可能表示到达末尾，也可能表示错误，因此调用前先将 errno 设为 0。
        errno = 0;
        struct dirent* const entry = ::readdir(directory);
        if (entry == nullptr)
        {
            if (errno != 0)
            {
                std::perror("readdir child failed");
            }
            break;
        }

        // d_name 是以空字符结尾的目录项名称；"." 和 ".." 分别代表当前目录和父目录。
        if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
        {
            continue;
        }

        // d_ino 是 inode 编号。d_type 是 Linux 提供的类型提示，可能为 DT_UNKNOWN；不能在所有文件系统上完全依赖它。
        const char* type_name = "unknown";
        switch (entry->d_type)
        {
        case DT_REG:
            type_name = "regular file";
            break;
        case DT_DIR:
            type_name = "directory";
            break;
        case DT_LNK:
            type_name = "symbolic link";
            break;
        case DT_UNKNOWN:
            type_name = "unknown";
            break;
        default:
            type_name = "other";
            break;
        }
        std::cout << "directory entry: name=" << entry->d_name
                  << ", inode=" << entry->d_ino
                  << ", type=" << type_name << '\n';
    }

    // closedir(dirp)：关闭目录流并释放与 DIR* 关联的资源。
    if (::closedir(directory) == -1)
    {
        std::perror("closedir child failed");
        cleanup();
        return;
    }

    // unlink(pathname)：删除一个文件名（目录项）。这里删除的是普通文件；对目录通常会失败。
    if (::unlink(unlink_file_path.c_str()) == -1)
    {
        std::perror("unlink unlink_demo.txt failed");
        cleanup();
        return;
    }
    std::cout << "unlink removed: " << unlink_file_path << '\n';

    // 创建供 remove() 删除的普通文件。
    file = std::fopen(remove_file_path.c_str(), "w");
    if (file == nullptr)
    {
        std::perror("fopen remove_demo.txt failed");
        cleanup();
        return;
    }
    std::fputs("this file will be removed by remove\n", file);
    if (std::fclose(file) != 0)
    {
        std::perror("fclose remove_demo.txt failed");
        cleanup();
        return;
    }

    // remove(pathname)：可删除普通文件；若 pathname 是空目录，也可删除该目录。
    if (std::remove(remove_file_path.c_str()) != 0)
    {
        std::perror("remove remove_demo.txt failed");
        cleanup();
        return;
    }
    std::cout << "remove deleted file: " << remove_file_path << '\n';

    // 创建空目录后用 remove() 演示目录删除；非空目录必须先删除内部所有条目。
    if (::mkdir(remove_directory_path.c_str(), 0700) == -1)
    {
        std::perror("mkdir remove_empty_directory failed");
        cleanup();
        return;
    }
    if (std::remove(remove_directory_path.c_str()) != 0)
    {
        std::perror("remove empty directory failed");
        cleanup();
        return;
    }
    std::cout << "remove deleted empty directory: " << remove_directory_path << '\n';

    // rmdir(pathname)：只能删除空目录；此时 child 已没有文件，因此可以删除。
    if (::rmdir(child_path.c_str()) == -1)
    {
        std::perror("rmdir child failed");
        cleanup();
        return;
    }
    if (::rmdir(root_path.c_str()) == -1)
    {
        std::perror("rmdir root failed");
        return;
    }
    std::cout << "rmdir removed all demo directories\n";
}
