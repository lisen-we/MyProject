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
#include "examples/msg_sharedTest.h"
#include "examples/Thread_Test.h"
#include "examples/Thread_Test_1.h"
#include "examples/mutiThread_EchoServer.h"
#include "examples/IO_Select_Test.h"
#include "examples/IO_epoll_Test.h"
#include "examples/MyEchoServer.h"
#include <iostream>

int main(int argc, char* argv[])
{
    // 【字符串转数值示例】
    // 知识点：atoi/strtol、进制、errno、end_ptr、C++ 异常。
    // 运行行为：使用函数内部的示例字符串完成数值转换。
    // 学习重点：优先使用 strto* 或 stoi/stod，并检查错误与未转换后缀。
    // str_to_num_fun();

    // 【数值转字符串示例】
    // 知识点：ecvt/fcvt/gcvt、std::to_string、ostringstream。
    // 运行行为：将函数内部的示例数值按不同方式转换为字符串。
    // 学习重点：旧式转换函数的静态缓冲区限制，以及 C++ 格式化方式。
    // num_to_str_fun();

    // 【格式化输出示例】
    // 知识点：printf/snprintf/fprintf、格式占位符、FILE*。
    // 运行行为：向当前工作目录的 test.txt 写入示例文本。
    // 学习重点：格式占位符必须匹配实参类型，snprintf 可避免缓冲区越界。
    // print_fun();

    // 【格式化输入示例】
    // 知识点：scanf、地址传参、输入格式。
    // 运行行为：从标准输入读取字符、整数和浮点数。
    // 学习重点：scanf 的格式符要与目标变量类型匹配，并检查返回值。
    // scanf_fun();

    // 【用户与组身份示例】
    // 知识点：UID/EUID、GID/EGID、setuid、setgid、Setuid/Setgid 位。
    // 运行行为：尝试切换至 ID 0，结果取决于当前有效权限。
    // 学习重点：真实 ID 与有效 ID 的区别，以及普通用户不能任意提升权限。
    // get_uid();

    // 【会话控制示例】
    // 知识点：fork、setsid、PID/PPID/PGID/SID、waitpid。
    // 运行行为：子进程调用 setsid() 创建新会话。
    // 学习重点：setsid 不改变父子关系；成功后 SID 与 PGID 等于子进程 PID。
    // set_sid();

    // 【文件 I/O 示例】
    // 知识点：open/write/fsync/lseek/read/close、文件描述符、errno。
    // 运行行为：创建或清空 /home/lisen/io_demo.txt，并执行读写操作。
    // 学习重点：处理部分写入、EINTR、文件偏移量和资源关闭。
    // IO_fun();

    // 【C 文件流与二进制记录示例】
    // 知识点：fopen、fileno、fdopen、ftell、fgetpos/fsetpos、fwrite/fread、feof、fflush。
    // 运行行为：创建 fopen_demo.txt、fdopen_demo.txt 和 student_record.bin。
    // 学习重点：FILE* 与 fd 的关系；不能直接保存包含 std::string 的对象。
    // file_stream_fun();

    // 【目录与目录项处理示例】
    // 知识点：mkdir、opendir/readdir/closedir、rmdir、remove、chown、chmod、unlink、umask。
    // 运行行为：在 /tmp 创建唯一测试目录，完成演示后清理示例创建的内容。
    // 学习重点：删除目录前必须为空；普通用户不能任意 chown 为其他用户。
    // directory_fun();

    // 【exec 与进程终止示例】
    // 知识点：fork、exec*、waitpid、exit/_exit、setjmp/sigsetjmp。
    // 运行行为：子进程执行 echo/env 等安全的目标程序。
    // 学习重点：exec 成功不会返回；父进程必须使用 waitpid 回收指定子进程。
    // process_control_fun();

    // 【进程组与优先级示例】
    // 知识点：getpgid/getpgrp、setpgid/setpgrp、getpriority/setpriority、nice。
    // 运行行为：在子进程中改变进程组和 nice 值，避免影响当前终端。
    // 学习重点：nice 值越小优先级越高；getpriority 返回 -1 时需结合 errno 判断。
    // process_group_priority_fun();

    // 【等待任意子进程示例】
    // 知识点：wait、SIGCHLD、僵尸进程、WIFEXITED/WEXITSTATUS。
    // 运行行为：子进程以退出码 24 结束，父进程通过 wait 解析状态字。
    // 学习重点：wait 等待任意子进程；waitpid 可等待指定子进程。
    // wait_fun();

    // 【单次 TCP 通信示例】
    // 知识点：socket、bind、listen、accept、connect、read/write、close、fork。
    // 运行行为：父进程在本机 9527 端口启动服务端，子进程连接后接收问候消息。
    // 学习重点：连接建立流程，以及监听 socket 与已连接 socket 的区别。
    // TCP_test();

    // 【多客户端 TCP 通信示例】
    // 知识点：fork、socket、bind、listen、accept、connect、read/write、close。
    // 运行行为：父进程依次处理三个本机客户端连接，子进程连续发送消息。
    // 学习重点：TCP 字节流可能发生部分读写，每个已连接 socket 都应关闭。
    // TCPV1();

    // 【交互式 TCP 回声服务示例】
    // 知识点：fork、socket、bind、listen、accept、connect、read/write、shutdown。
    // 运行行为：父进程运行回声服务，子进程从终端读取内容并接收回显。
    // 学习重点：一个 TCP 连接可连续收发多条消息；read 的返回字节数才有效。
    // EchoServe();

    // 【中缀表达式 TCP 计算服务示例】
    // 知识点：中缀转后缀、栈求值、TCP 消息边界、客户端/服务端分工。
    // 运行行为：客户端发送表达式，服务端计算后返回结果；输入 quit 退出。
    // 学习重点：错误信息不能再次作为表达式计算；TCP 应用层需要协议边界。
    // calculationServerTest();

    // 【UDP 回声服务示例】
    // 知识点：SOCK_DGRAM、bind、sendto、recvfrom、无连接数据报通信。
    // 运行行为：服务端绑定本机 9999 端口，客户端收发 UDP 回声数据报。
    // 学习重点：UDP 没有 listen/accept；客户端退出不会自动通知服务端。
    // EchoServeUDP();

    // 【TCP 与 UDP 套接字类型示例】
    // 知识点：SOCK_STREAM、SOCK_DGRAM、getsockopt、SO_TYPE。
    // 运行行为：创建 TCP/UDP 套接字并读取各自的 SO_TYPE 选项值。
    // 学习重点：TCP 面向连接且可靠，UDP 面向数据报且无连接。
    // UDP_TCP_type();

    // 【SO_REUSEADDR 与 TCP 服务重启示例】
    // 知识点：setsockopt、SO_REUSEADDR、bind、listen、accept、TIME_WAIT。
    // 运行行为：对比设置与未设置 SO_REUSEADDR 时的快速重启结果。
    // 学习重点：SO_REUSEADDR 必须在 bind 前设置，且不能抢占正在监听的端口。
    // ReuseAddrEchoServerExample();

    // 【进程与线程信号示例】
    // 知识点：sigaction、SIGALRM、SIGINT、alarm、pause、getpid、pthread_self。
    // 运行行为：每两秒接收一次 SIGALRM；按 Ctrl+C 可提前结束。
    // 学习重点：信号处理函数只能做异步信号安全操作，业务逻辑应在正常流程处理。
    // SignalThreadExample();

    // 【多进程并发 TCP 服务示例】
    // 知识点：pipe、fork、accept、子进程并发、TCP 半关闭、waitpid。
    // 运行行为：启动器等待服务端就绪后创建多个独立客户端进行回声通信。
    // 学习重点：管道可消除启动竞争；每个进程应关闭无用的文件描述符副本。
    // ConcurrentServer();

    // 【单向管道通信示例】
    // 知识点：pipe、fork、poll、读写端关闭、EOF。
    // 运行行为：子进程向管道写入消息，父进程轮询并读取消息。
    // 学习重点：关闭未使用的写端，读取端才能在写端全部关闭后得到 EOF。
    // singlePipeTest();

    // 【双向管道通信示例】
    // 知识点：两条单向 pipe、父子进程、read/write、关闭无用端、waitpid。
    // 运行行为：父进程发送请求，子进程读取后通过另一条管道返回响应。
    // 学习重点：一个 pipe 只能单向传输；双向通信需要方向相反的两条管道。
    // DoublePipeTest();

    // 【System V 共享内存示例】
    // 知识点：shmget/shmat/shmdt/shmctl、semget/semop/semctl、fork。
    // 运行行为：父子进程交替向共享内存中的学生数组写入测试记录。
    // 学习重点：共享内存不提供同步；必须借助信号量协调访问并回收 IPC 资源。
    // shmgetTest();

    // 【System V 消息队列示例】
    // 知识点：msgget/msgsnd/msgrcv/msgctl、消息类型、fork、waitpid。
    // 运行行为：子进程发送一条学生信息，父进程按类型接收后删除队列。
    // 学习重点：消息类型不属于消息正文；仅在通信完成后回收消息队列。
    // msg_sharedTest();

    // 【POSIX 线程基础示例】
    // 知识点：pthread_create/pthread_join、互斥量、信号量、线程退出与取消。
    // 运行行为：依次运行创建传参、同步、主动退出和延迟取消等阶段。
    // 学习重点：编译与链接均需 -pthread；共享数据需要同步机制保护。
    // Thread_Test();

    // 【多线程 TCP 聊天服务器示例】
    // 知识点：accept、pthread、互斥量、TCP 字节流分帧、广播、断开清理。
    // 运行行为：监听 10000 端口；客户端可用 nc 发送换行结尾的聊天消息。
    // 学习重点：一个客户端一个分离线程；TCP 数据必须按换行符重新组帧。
    // mutiThread_EchoServer();

    // 【select I/O 多路复用回声服务示例】
    // 知识点：fd_set、select、max_fd + 1、超时重置、多个客户端。
    // 运行行为：传入 s 启动服务端，传入 c 启动客户端。
    // 学习重点：select 每次调用后会修改 fd_set 和超时值，下一轮必须重新设置。
    // IOSelectTest(argv[1]);

    // 【epoll I/O 多路复用回声服务示例】
    // 知识点：epoll_create1、epoll_ctl、epoll_wait、非阻塞 socket、EPOLLOUT。
    // 运行行为：传入 s 启动服务端，传入 c 启动客户端。
    // 学习重点：TCP 写入可能部分完成；待发送数据应在 EPOLLOUT 事件中继续发送。
    // IO_EPOLL_TEST(argv[1]);

    // 【epoll TCP/UDP 聊天室】
    // 知识点：epoll、TCP accept/connect、UDP sendto/recvfrom、广播、应用层协议。
    // 运行行为：第一个客户端输入为姓名，之后输入为聊天消息；/quit 退出客户端。
    // 学习重点：TCP 按换行符分帧；UDP 以“IP:端口”标识聊天客户端。
    // 用法：./Linux_learning s TCP；另开终端执行 ./Linux_learning c TCP。
    //       将 TCP 替换为 UDP 可运行 UDP 版本。
    if (argc != 3)
    {
        std::cout << "Usage: " << argv[0] << " s|c TCP|UDP\n";
        return 1;
    }

    MyEchoServer(argv[1], argv[2]);
    return 0;
}
