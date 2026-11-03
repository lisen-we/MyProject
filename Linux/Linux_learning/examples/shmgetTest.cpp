#include "shmgetTest.h"

#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{
constexpr std::size_t kStudentCapacity = 6;

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
    // 单一互斥锁只能保证互斥，不能决定下一次由哪个进程获得锁。
    // 这两个信号量分别表示父进程和子进程的执行轮次。
    sem_t parent_turn;
    sem_t child_turn;

    std::size_t count;
    Student data[kStudentCapacity];
};

bool wait_semaphore(sem_t* semaphore, const char* error_message)
{
    while (::sem_wait(semaphore) == -1)
    {
        // sem_wait 正常情况下会阻塞而不占用 CPU。只有被信号中断时才重试，
        // 其他错误必须报告，不能无条件循环空转。
        if (errno == EINTR)
            continue;

        std::perror(error_message);
        return false;
    }

    return true;
}

bool post_semaphore(sem_t* semaphore, const char* error_message)
{
    if (::sem_post(semaphore) == -1)
    {
        std::perror(error_message);
        return false;
    }

    return true;
}

bool append_students(SharedStudents* shared,
    const StudentInput* students,
    std::size_t student_count,
    sem_t* current_process_turn,
    sem_t* next_process_turn)
{
    for (std::size_t index = 0; index < student_count; ++index)
    {
        // 父、子进程只能在轮到自己时继续执行，因此写入顺序必定是
        // 父、子、父、子……，而不是由调度器随机决定。
        if (!wait_semaphore(current_process_turn, "sem_wait failed"))
            return false;

        if (shared->count >= kStudentCapacity)
        {
            std::fputs("shared-memory student array is full\n", stderr);
            return false;
        }

        Student& target = shared->data[shared->count];
        target.age = students[index].age;
        std::strncpy(target.name, students[index].name, sizeof(target.name) - 1);
        target.name[sizeof(target.name) - 1] = '\0';
        ++shared->count;

        // 必须在当前记录完整写入后，才通知另一个进程进入下一轮。
        // sem_post 同时保证了进程之间所需的内存可见性顺序。
        if (!post_semaphore(next_process_turn, "sem_post failed"))
            return false;
    }

    return true;
}

bool wait_for_child(pid_t child_pid)
{
    int child_status = 0;
    pid_t waited_pid = -1;
    do
    {
        waited_pid = ::waitpid(child_pid, &child_status, 0);
    } while (waited_pid == -1 && errno == EINTR);

    if (waited_pid == -1)
    {
        std::perror("waitpid failed");
        return false;
    }

    if (!WIFEXITED(child_status) || WEXITSTATUS(child_status) != EXIT_SUCCESS)
    {
        std::fputs("child process did not finish the shared-memory demo successfully\n", stderr);
        return false;
    }

    return true;
}

void destroy_shared_memory(int shmid, SharedStudents* shared)
{
    // 仅由父进程回收资源，并且必须先 waitpid()。子进程仍可能使用共享
    // 信号量时就销毁它，会产生未定义行为。
    if (::sem_destroy(&shared->parent_turn) == -1)
        std::perror("destroy parent_turn failed");
    if (::sem_destroy(&shared->child_turn) == -1)
        std::perror("destroy child_turn failed");
    if (::shmdt(shared) == -1)
        std::perror("shmdt failed");
    if (::shmctl(shmid, IPC_RMID, nullptr) == -1)
        std::perror("shmctl IPC_RMID failed");
}
}

void shmgetTest()
{
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

    // 父进程必须在 fork() 之前创建共享内存并初始化全部信号量；否则子进程
    // 可能抢先执行，并对未初始化的 sem_t 调用 sem_wait()。
    // IPC_PRIVATE 表示每次请求一块新的共享内存，而不是按公开 key 查找。
    // 下方创建的子进程会继承 shmid，以及当前已挂载的共享内存映射。
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
    if (::sem_init(&shared->parent_turn, 1, 1) == -1)
    {
        std::perror("initialize parent_turn failed");
        ::shmdt(shared);
        ::shmctl(shmid, IPC_RMID, nullptr);
        return;
    }
    if (::sem_init(&shared->child_turn, 1, 0) == -1)
    {
        std::perror("initialize child_turn failed");
        ::sem_destroy(&shared->parent_turn);
        ::shmdt(shared);
        ::shmctl(shmid, IPC_RMID, nullptr);
        return;
    }

    const pid_t child_pid = ::fork();
    if (child_pid == -1)
    {
        std::perror("fork failed");
        destroy_shared_memory(shmid, shared);
        return;
    }

    if (child_pid == 0)
    {
        const bool child_success = append_students(shared,
            kChildStudents.data(),
            kChildStudents.size(),
            &shared->child_turn,
            &shared->parent_turn);

        // 子进程只解除自身的地址映射，不能销毁共享信号量或删除共享内存；
        // 这些资源由父进程统一回收。
        if (::shmdt(shared) == -1)
            std::perror("child shmdt failed");
        _exit(child_success ? EXIT_SUCCESS : EXIT_FAILURE);
    }

    const bool parent_success = append_students(shared,
        kParentStudents.data(),
        kParentStudents.size(),
        &shared->parent_turn,
        &shared->child_turn);
    const bool child_success = wait_for_child(child_pid);

    if (parent_success && child_success)
    {
        // 在 waitpid() 之后统一输出，避免父子进程各自的 stdout 缓冲区混合，
        // 从而准确展示共享数组中的实际交替顺序。
        std::puts("shared-memory students (parent and child alternate):");
        for (std::size_t index = 0; index < shared->count; ++index)
        {
            std::printf("  %zu. name: %s, age: %d\n",
                index + 1,
                shared->data[index].name,
                shared->data[index].age);
        }
    }

    destroy_shared_memory(shmid, shared);
}
