#include "shmgetTest.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

void shmgetTest()
{
    constexpr std::size_t kStudentCapacity = 6;
    constexpr unsigned short kParentTurn = 0;
    constexpr unsigned short kChildTurn = 1;

    struct Student
    {
        int age;
        char name[64];
    };

    struct StudentInput
    {
        int age;
        const char* name;
    };

    struct SharedStudents
    {
        // System V 信号量由内核维护，因此共享内存只保存需要共享的数据。
        std::size_t count;
        Student data[kStudentCapacity];
    };

    // glibc 不会替应用程序声明 semctl 所需的 union semun，调用 SETALL 时
    // 由应用程序声明包含 array 成员的联合体即可。
    union Semun
    {
        unsigned short* array;
    };

    constexpr std::array<StudentInput, 3> kParentStudents = {
        StudentInput{21, "xiaoming"},
        StudentInput{23, "xiaohong"},
        StudentInput{25, "xiaoli"},
    };
    constexpr std::array<StudentInput, 3> kChildStudents = {
        StudentInput{22, "xiaozhang"},
        StudentInput{24, "xiaohei"},
        StudentInput{26, "xiaozi"},
    };

    // IPC_PRIVATE 每次创建一块新的共享内存；父进程 fork 后，子进程会继承
    // shmid 以及当前的共享内存映射，因此不需要用 ftok 生成公开 key。
    const int shmid = ::shmget(IPC_PRIVATE, sizeof(SharedStudents), IPC_CREAT | 0600);
    if (shmid == -1)
    {
        std::perror("shmget failed");
        return;
    }

    auto* shared = static_cast<SharedStudents*>(::shmat(shmid, nullptr, 0));
    if (shared == reinterpret_cast<void*>(-1))
    {
        std::perror("shmat failed");
        ::shmctl(shmid, IPC_RMID, nullptr);
        return;
    }
    shared->count = 0;

    // 创建包含两个信号量的 System V 信号量集：
    // kParentTurn 初始为 1，父进程可先写；kChildTurn 初始为 0，子进程等待。
    const int semid = ::semget(IPC_PRIVATE, 2, IPC_CREAT | 0600);
    if (semid == -1)
    {
        std::perror("semget failed");
        ::shmdt(shared);
        ::shmctl(shmid, IPC_RMID, nullptr);
        return;
    }

    std::array<unsigned short, 2> initial_values = {1, 0};
    Semun semun {};
    semun.array = initial_values.data();
    if (::semctl(semid, 0, SETALL, semun) == -1)
    {
        std::perror("semctl SETALL failed");
        ::semctl(semid, 0, IPC_RMID);
        ::shmdt(shared);
        ::shmctl(shmid, IPC_RMID, nullptr);
        return;
    }

    // 所有 IPC 对象必须在 fork() 前创建和初始化。否则子进程可能在父进程
    // 设置初始信号量值之前就开始执行，从而无法保证父子进程的交替关系。
    const pid_t child_pid = ::fork();
    if (child_pid == -1)
    {
        std::perror("fork failed");
        ::semctl(semid, 0, IPC_RMID);
        ::shmdt(shared);
        ::shmctl(shmid, IPC_RMID, nullptr);
        return;
    }

    if (child_pid == 0)
    {
        bool child_success = true;

        for (const StudentInput& student : kChildStudents)
        {
            // sem_op 为 -1 是 P 操作：当 kChildTurn 为 0 时阻塞；获得子进程
            // 轮次后将其减为 0。仅在 EINTR 时重试，避免错误时无意义地空转。
            sembuf wait_child_turn {};
            wait_child_turn.sem_num = kChildTurn;
            wait_child_turn.sem_op = -1;
            wait_child_turn.sem_flg = 0;
            while (::semop(semid, &wait_child_turn, 1) == -1)
            {
                if (errno == EINTR)
                    continue;

                std::perror("child semop wait failed");
                child_success = false;
                break;
            }
            if (!child_success)
                break;

            if (shared->count >= kStudentCapacity)
            {
                std::fputs("shared-memory student array is full\n", stderr);
                child_success = false;
                break;
            }

            Student& target = shared->data[shared->count];
            target.age = student.age;
            std::strncpy(target.name, student.name, sizeof(target.name) - 1);
            target.name[sizeof(target.name) - 1] = '\0';
            ++shared->count;

            // sem_op 为 +1 是 V 操作：当前记录完整写入后，通知父进程开始
            // 下一轮。内核信号量同时保证写入结果对父进程可见。
            sembuf post_parent_turn {};
            post_parent_turn.sem_num = kParentTurn;
            post_parent_turn.sem_op = 1;
            post_parent_turn.sem_flg = 0;
            while (::semop(semid, &post_parent_turn, 1) == -1)
            {
                if (errno == EINTR)
                    continue;

                std::perror("child semop post failed");
                child_success = false;
                break;
            }
            if (!child_success)
                break;
        }

        // 子进程只解除自己的共享内存映射。System V 信号量集与共享内存均由
        // 父进程在 waitpid() 之后统一删除，避免子进程过早回收资源。
        if (::shmdt(shared) == -1)
        {
            std::perror("child shmdt failed");
            child_success = false;
        }
        _exit(child_success ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    bool parent_success = true;
    for (const StudentInput& student : kParentStudents)
    {
        // sem_op 为 -1 是 P 操作：父进程只在 kParentTurn 大于 0 时继续执行。
        sembuf wait_parent_turn {};
        wait_parent_turn.sem_num = kParentTurn;
        wait_parent_turn.sem_op = -1;
        wait_parent_turn.sem_flg = 0;
        while (::semop(semid, &wait_parent_turn, 1) == -1)
        {
            if (errno == EINTR)
                continue;

            std::perror("parent semop wait failed");
            parent_success = false;
            break;
        }
        if (!parent_success)
            break;

        if (shared->count >= kStudentCapacity)
        {
            std::fputs("shared-memory student array is full\n", stderr);
            parent_success = false;
            break;
        }

        Student& target = shared->data[shared->count];
        target.age = student.age;
        std::strncpy(target.name, student.name, sizeof(target.name) - 1);
        target.name[sizeof(target.name) - 1] = '\0';
        ++shared->count;

        // 父进程写完一个学生后执行 V 操作，让子进程获得下一次写入轮次。
        sembuf post_child_turn {};
        post_child_turn.sem_num = kChildTurn;
        post_child_turn.sem_op = 1;
        post_child_turn.sem_flg = 0;
        while (::semop(semid, &post_child_turn, 1) == -1)
        {
            if (errno == EINTR)
                continue;

            std::perror("parent semop post failed");
            parent_success = false;
            break;
        }
        if (!parent_success)
            break;
    }

    // 父进程自身异常退出时，子进程可能仍在 semop 中等待自己的轮次；先终止
    // 子进程，再 waitpid 回收，避免父进程在错误分支永久等待。
    if (!parent_success && ::kill(child_pid, SIGTERM) == -1 && errno != ESRCH)
        std::perror("kill child failed");

    int child_status = 0;
    pid_t waited_pid = -1;
    do
    {
        waited_pid = ::waitpid(child_pid, &child_status, 0);
    } while (waited_pid == -1 && errno == EINTR);

    const bool child_success = waited_pid == child_pid && WIFEXITED(child_status) &&
        WEXITSTATUS(child_status) == EXIT_SUCCESS;
    if (waited_pid == -1)
        std::perror("waitpid failed");
    else if (!child_success)
        std::fputs("child process did not finish the shared-memory demo successfully\n", stderr);

    if (parent_success && child_success)
    {
        // 等子进程退出后再输出，可避免两个进程独立缓冲 stdout 造成的显示顺序
        // 混乱；此处展示的是共享数组中真正的写入顺序。
        std::puts("shared-memory students (parent and child alternate):");
        for (std::size_t index = 0; index < shared->count; ++index)
        {
            std::printf("  %zu. name: %s, age: %d\n",
                index + 1,
                shared->data[index].name,
                shared->data[index].age);
        }
    }

    // IPC_RMID 不会自动创建或分配新的 IPC 对象，而是把现有内核 IPC 对象
    // 标记为删除。此时子进程已退出，因此父进程可以安全地回收信号量集和共享内存。
    if (::semctl(semid, 0, IPC_RMID) == -1)
        std::perror("semctl IPC_RMID failed");
    if (::shmdt(shared) == -1)
        std::perror("parent shmdt failed");
    if (::shmctl(shmid, IPC_RMID, nullptr) == -1)
        std::perror("shmctl IPC_RMID failed");
}
