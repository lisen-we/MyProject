#include "msg_sharedTest.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void msg_sharedTest()
{
    constexpr long kStudentMessageType = 1;

    struct StudentData
    {
        int id;
        int age;
        char name[64];
        char message[256];
    };

    struct Message
    {
        // System V 消息队列要求第一个字段必须是 long 类型的消息类型，
        // 且发送时该值必须大于 0。它不属于消息正文的长度。
        long type;
        StudentData data;
    };

    // IPC_PRIVATE 每次都创建一个新的队列，并不表示该队列只能被一个进程
    // 使用。本示例在 fork() 前创建队列，子进程会继承 msqid，因此不需要
    // ftok() 生成供无关进程查找的公开 key。
    const int msqid = ::msgget(IPC_PRIVATE, IPC_CREAT | 0600);
    if (msqid == -1)
    {
        std::perror("msgget failed");
        return;
    }

    const pid_t child_pid = ::fork();
    if (child_pid == -1)
    {
        std::perror("fork failed");
        ::msgctl(msqid, IPC_RMID, nullptr);
        return;
    }

    if (child_pid == 0)
    {
        // 子进程是发送方：构造一条类型为 1 的学生信息消息。
        Message send_message {};
        send_message.type = kStudentMessageType;
        send_message.data.id = 666;
        send_message.data.age = 15;
        std::strncpy(send_message.data.name, "xiaoming", sizeof(send_message.data.name) - 1);
        std::strncpy(send_message.data.message, "bingo", sizeof(send_message.data.message) - 1);

        // 第三个参数只计算 type 后面的消息正文长度。msgsnd 默认在队列满时
        // 阻塞；若阻塞过程被信号中断，只在 errno == EINTR 时重试。
        bool send_success = false;
        while (!send_success)
        {
            if (::msgsnd(msqid, &send_message, sizeof(send_message.data), 0) == 0)
            {
                send_success = true;
                break;
            }

            if (errno == EINTR)
                continue;

            std::perror("msgsnd failed");
            break;
        }

        // 子进程只能发送并退出，不能删除消息队列；否则父进程可能还未来得及
        // 调用 msgrcv()，队列就已经被提前移除了。
        _exit(send_success ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    // 父进程是接收方。第四个参数指定只接收 type 为 1 的消息；若暂时没有
    // 这类消息，msgrcv 会阻塞，而不是不停循环占用 CPU。
    Message receive_message {};
    bool receive_success = false;
    while (!receive_success)
    {
        if (::msgrcv(msqid,
            &receive_message,
            sizeof(receive_message.data),
            kStudentMessageType,
            0) != -1)
        {
            receive_success = true;
            break;
        }

        if (errno == EINTR)
            continue;

        std::perror("msgrcv failed");
        break;
    }

    // waitpid 回收子进程，避免产生僵尸进程；同时保证删除队列之前发送方
    // 已经彻底结束。
    int child_status = 0;
    pid_t waited_pid = -1;
    while (waited_pid == -1)
    {
        waited_pid = ::waitpid(child_pid, &child_status, 0);
        if (waited_pid != -1)
            break;

        if (errno == EINTR)
            continue;

        std::perror("waitpid failed");
        break;
    }

    const bool child_success = waited_pid == child_pid && WIFEXITED(child_status) &&
        WEXITSTATUS(child_status) == EXIT_SUCCESS;
    if (waited_pid != -1 && !child_success)
        std::fputs("child process did not send the message successfully\n", stderr);

    if (receive_success && child_success)
    {
        std::printf("student id: %d, name: %s, age: %d, message: %s\n",
            receive_message.data.id,
            receive_message.data.name,
            receive_message.data.age,
            receive_message.data.message);
    }

    // 仅由父进程在接收完成并回收子进程之后删除队列，避免遗留内核 IPC
    // 资源，也避免发送方、接收方并发操作期间提前删除队列。
    if (::msgctl(msqid, IPC_RMID, nullptr) == -1)
        std::perror("msgctl IPC_RMID failed");
}
