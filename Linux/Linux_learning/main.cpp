#include "examples/FileIoExample.h"
#include "examples/FileStreamExample.h"
#include "examples/DirectoryExample.h"
#include "examples/IdentityExample.h"
#include "examples/NumberToStringExample.h"
#include "examples/PrintExample.h"
#include "examples/ProcessControlExample.h"
#include "examples/ProcessGroupPriorityExample.h"
#include "examples/ScanfExample.h"
#include "examples/SessionExample.h"
#include "examples/StringToNumberExample.h"
#include "examples/WaitExample.h"
#include "./socket_test.h"
int main()
{
    // 【字符串转数值】知识点：atoi/strtol、进制、errno、end_ptr、C++ 异常。
    // 参数：无；示例数据写在函数内部。
    // 需掌握：优先使用 strto* 或 stoi/stod，并检查错误与未转换后缀。
    // str_to_num_fun();

    // 【数值转字符串】知识点：ecvt/fcvt/gcvt、std::to_string、ostringstream。
    // 参数：无；示例数值和精度写在函数内部。
    // 需掌握：旧式转换函数的静态缓冲区限制，以及 C++ 格式化方式。
    // num_to_str_fun();

    // 【格式化输出】知识点：printf/snprintf/fprintf、格式占位符、FILE*。
    // 参数：无；函数会向当前工作目录的 test.txt 写入示例文本。
    // 需掌握：格式占位符必须匹配实参类型，snprintf 可避免缓冲区越界。
    // print_fun();

    // 【格式化输入】知识点：scanf、地址传参、输入格式。
    // 参数：无；运行后从标准输入读取字符、整数和浮点数。
    // 需掌握：scanf 的格式符要与目标变量类型匹配，并检查返回值。
    // scanf_fun();

    // 【用户与组身份】知识点：UID/EUID、GID/EGID、setuid、setgid、Setuid/Setgid 位。
    // 参数：无；会尝试切换至 ID 0，结果取决于当前有效权限。
    // 需掌握：真实 ID 与有效 ID 的区别，以及普通用户不能任意提升权限。
    // get_uid();

    // 【会话控制】知识点：fork、setsid、PID/PPID/PGID/SID、waitpid。
    // 参数：无；子进程调用 setsid() 创建新会话。
    // 需掌握：setsid 不改变父子关系；成功后 SID 与 PGID 等于子进程 PID。
    // set_sid();

    // 【文件 I/O】知识点：open/write/fsync/lseek/read/close、文件描述符、errno。
    // 参数：无；函数会创建或清空 /home/lisen/io_demo.txt。
    // 需掌握：处理部分写入、EINTR、文件偏移量和资源关闭。
    // IO_fun();

    // 【C 文件流与二进制记录】知识点：fopen、fileno、fdopen、ftell、fgetpos/fsetpos、fwrite/fread、feof/clearerr、fflush、fclose。
    // 参数：无；函数会创建或清空 fopen_demo.txt、fdopen_demo.txt 和 student_record.bin。
    // 需掌握：FILE* 与 fd 的关系；fread/fwrite 只适合平凡可复制类型，不能直接保存 std::string 等对象。
     //file_stream_fun();

    // 【目录与目录项处理】知识点：mkdir、opendir/readdir/closedir、rmdir、remove、chown、chmod、unlink、umask。
    // 参数：无；函数在 /tmp 创建唯一测试目录，完成后删除本示例创建的文件和空目录。
    // 需掌握：rmdir/remove 删除目录时目录必须为空；普通用户不能随意 chown 为其他用户。
    // directory_fun();

    // 【exec 与进程终止】知识点：fork、exec*、waitpid、exit/_exit、setjmp/sigsetjmp。
    // 参数：无；内部使用 echo/env 作为安全的目标程序，并在子进程中演示终止行为。
    // 需掌握：exec 成功不会返回；父进程必须使用 waitpid 回收指定子进程。
    //process_control_fun();

    // 【进程组与优先级】知识点：getpgid/getpgrp、setpgid/setpgrp、getpriority/setpriority/nice。
    // 参数：无；会在子进程中改变进程组和 nice 值，避免影响当前终端。
    // 需掌握：nice 值越小优先级越高；getpriority 返回 -1 时需结合 errno 判断。
    //process_group_priority_fun();

    // 【等待任意子进程】知识点：wait、SIGCHLD、僵尸进程、WIFEXITED/WEXITSTATUS。
    // 参数：无；子进程以退出码 24 结束，父进程通过 wait 解析状态字。
    // 需掌握：wait 等待任意子进程，waitpid 可指定目标 PID。
    //wait_fun();
    TCP_test();
    return 0;
}
