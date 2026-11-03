#include "ConcurrentServer.h"
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
#include "examples/SignalThreadExample.h"
#include "examples/StringToNumberExample.h"
#include "examples/WaitExample.h"
#include "examples/SocketTestExample.h"
#include "examples/EchoServerExample.h"
#include "examples/CalculationServer.h"
#include "examples/EchoServer_UDP.h"
#include "examples/ReuseAddrEchoServerExample.h"
#include "examples/UDP_TCP_type.h"
#include "examples/Pipe_test.h"
#include "examples/shmgetTest.h"
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
    // 【单次 TCP 通信】知识点：socket、bind、listen、accept、connect、read/write、close、fork。
    // 参数：无；父进程在本机 9527 端口启动服务端，子进程连接后接收一条问候消息。
    // 需掌握：连接建立的基本流程，以及监听 Socket 与已连接 Socket 的区别。
    //TCP_test();

    // 【多客户端 TCP 通信】知识点：fork、socket、bind、listen、accept、connect、read/write、close。
    // 参数：无；父进程依次处理三个本机客户端连接，子进程连续创建三个客户端并发送消息。
    // 需掌握：TCP 字节流可能发生部分读写，且每个已连接 Socket 都应在使用后关闭。
    //TCPV1();

    // 【交互式 TCP 回声服务】知识点：fork、socket、bind、listen、accept、connect、read/write、shutdown。
    // 参数：无；父进程在本机 9527 端口运行回声服务，子进程从终端读取内容并回显，输入 quit 退出。
    // 需掌握：一个 TCP 连接可连续收发多条消息；read() 的返回字节数才是有效数据长度。
    //EchoServe();

    // 【中缀表达式 TCP 计算服务】知识点：中缀转后缀、栈求值、TCP 消息边界、客户端/服务端分工。
    // 参数：无；子进程读取中缀表达式并发送，父进程计算后返回结果，输入 quit 结束客户端。
    // 需掌握：错误信息不能再次作为表达式计算；TCP 应以换行符等协议边界拆分多条消息。
    // calculationServerTest();

    // 【UDP 回声服务】知识点：SOCK_DGRAM、bind、sendto、recvfrom、无连接数据报通信。
    // 参数：无；服务端绑定本机 9999 端口，客户端向 127.0.0.1:9999 发送数据报并接收回声。
    // 需掌握：UDP 没有 listen/accept；客户端输入 q 仅退出本地循环，不会自动通知服务端。
    // EchoServeUDP();

    // 【TCP 与 UDP 套接字类型】知识点：SOCK_STREAM、SOCK_DGRAM、getsockopt、SO_TYPE。
    // 参数：无；创建 TCP/UDP 套接字并读取其 SO_TYPE 选项值。
    // 需掌握：套接字类型决定通信语义，TCP 面向连接且可靠，UDP 面向数据报且无连接。
    //UDP_TCP_type();

    // 【SO_REUSEADDR 与 TCP 服务重启】知识点：setsockopt、SO_REUSEADDR、bind、listen、accept、TIME_WAIT。
    // 参数：无；第一轮服务结束后立即尝试不设置 SO_REUSEADDR 的 bind，再重启设置选项的服务端。
    // 需掌握：前者预期得到 EADDRINUSE；SO_REUSEADDR 要在 bind 前设置，仍不能抢占正在监听的端口。
    //ReuseAddrEchoServerExample();

    // 【进程与线程信号】知识点：sigaction、SIGALRM、SIGINT、alarm、pause、getpid、pthread_self。
    // 参数：无；每两秒接收一次 SIGALRM，默认演示三次；按 Ctrl+C 发送 SIGINT 后提前结束。
    // 需掌握：信号处理函数只能做异步信号安全操作，业务输出和线程 ID 查询应放在正常执行流程中。
    // SignalThreadExample();

    //ConcurrentServer();
    //singlePipeTest();

    // 【双向管道通信】知识点：两条单向 pipe、父子进程、read/write、关闭无用端、waitpid。
    // 参数：无；父进程发送请求，子进程读取后通过另一条 pipe 返回响应。
    // 需掌握：一个 pipe 只能单向传输；双向通信需要两条方向相反的管道。
    //DoublePipeTest();
    shmgetTest();
    return 0;
}
