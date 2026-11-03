#include "Thread_Test.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <time.h>
#include <unistd.h>

namespace
{
// 学习顺序：主线程创建工作线程 → 两者并发执行 → 同步共享数据 → join 回收。
// 线程共享同一个进程的地址空间、全局变量和文件描述符，但各自有独立的栈。
// 因而给线程传指针很方便，同时也必须保证对象的生命周期以及共享读写的同步。

void check_pthread(int error, const char* operation)
{
    // pthread_create/join/mutex 等函数：成功返回 0，失败直接返回错误码，
    // 不应按 -1 判断，也不能直接用 perror(errno) 解释它们的错误。
    if (error != 0)
    {
        std::fprintf(stderr, "%s: %s\n", operation, std::strerror(error));
        // 本学习示例遇到同步/创建失败就终止整个进程，避免继续使用无效资源。
        std::exit(EXIT_FAILURE);
    }
}

void check_system(int result, const char* operation)
{
    // sem_init/sem_wait/sem_post 等与 pthread 函数不同：失败返回 -1 并设置 errno。
    if (result == -1)
    {
        std::perror(operation);
        std::exit(EXIT_FAILURE);
    }
}

void wait_semaphore(sem_t* semaphore)
{
    // sem_wait 将计数减 1；计数为 0 时阻塞，直到其他线程 sem_post。
    // 被信号中断并不意味着获得了许可，必须在 EINTR 时重试。
    int result;
    do
    {
        result = ::sem_wait(semaphore);
    } while (result == -1 && errno == EINTR);
    check_system(result, "sem_wait");
}

struct ThreadArgument
{
    std::string message;
    int completed = 0;
};

void* thread_entry(void* arg)
{
    // pthread 的固定入口类型是 void* (*)(void*)。arg 就是 create 的第 4 个参数，
    // void* 不携带类型信息，必须按创建者实际传入的类型还原。
    ThreadArgument* argument = static_cast<ThreadArgument*>(arg);
    // pthread_self 返回当前线程的 pthread_t；它不等于 getpid() 返回的进程 PID。
    // pthread_t 是不透明类型，不假定它是整数；比较线程应使用 pthread_equal。
    const pthread_t self = ::pthread_self();
    for (int index = 0; index < 3; ++index)
    {
        std::printf("[worker, PID=%ld] %s, step=%d\n",
            static_cast<long>(::getpid()), argument->message.c_str(), index + 1);
        ++argument->completed;
    }
    std::printf("[worker] pthread_equal(self, self)=%d\n",
        ::pthread_equal(self, self) != 0);

    // 从入口 return 等价于 pthread_exit(返回指针)，只结束当前线程。
    // 返回的是指针，不是对象副本；这里对象属于调用者，join 前一直存活。
    // 绝不能 return 指向此线程栈上局部变量的地址。
    return arg;
}

void creation_example()
{
    std::puts("\n=== 1. pthread_create -> worker -> pthread_join ===");
    // 兼容 C++11：含成员默认初值的结构体不能在该标准下这样做聚合初始化。
    // 先默认构造（completed 已为 0），再给 message 赋值。
    ThreadArgument argument;
    argument.message = "hello from main";
    pthread_t thread;
    // 参数依次是：输出线程标识的地址、线程属性、入口函数、传给入口的指针。
    // nullptr 属性表示默认属性（可 join）。新线程可能在 create 返回前就已执行。
    // 与 fork 不同，create 在主线程返回；新线程从 thread_entry 开始执行。
    check_pthread(::pthread_create(&thread, nullptr, thread_entry, &argument),
        "pthread_create");
    std::printf("[main, PID=%ld] created worker; now joining\n",
        static_cast<long>(::getpid()));

    void* result = nullptr;
    // join 等待指定线程结束，将入口返回的 void* 写到 result，并回收线程资源。
    // join 不会启动线程；线程在 create 成功后已经可以运行。只能 join 一次，
    // 也不能 join 自己。这里不会像 waitpid 那样回收子进程。
    check_pthread(::pthread_join(thread, &result), "pthread_join");
    // 主线程在 join 成功后才读 completed，无需为该结果再加互斥锁。
    // argument 放在当前函数而非某个已返回的 create() 辅助函数里，避免悬空指针。
    const ThreadArgument* returned = static_cast<ThreadArgument*>(result);
    std::printf("[main] joined: message=%s, completed=%d\n",
        returned->message.c_str(), returned->completed);
}

struct Counter
{
    pthread_mutex_t mutex;
    int value = 0;
};

void* count_entry(void* arg)
{
    Counter* counter = static_cast<Counter*>(arg);
    for (int index = 0; index < 10000; ++index)
    {
        // ++value 包含读、修改、写，不能假定它是一个不可分割的操作。
        // 两个线程都必须用同一把锁：lock 成功后才可进入临界区；
        // 若其他线程持锁，当前线程等待。unlock 后其他线程才能进入。
        check_pthread(::pthread_mutex_lock(&counter->mutex), "pthread_mutex_lock");
        ++counter->value;
        check_pthread(::pthread_mutex_unlock(&counter->mutex), "pthread_mutex_unlock");
    }
    return nullptr;
}

void mutex_example()
{
    std::puts("\n=== 2. mutex: two workers increment one shared counter ===");
    Counter counter {};
    // 动态初始化互斥量；nullptr 表示默认属性。先初始化，再把地址传给线程。
    check_pthread(::pthread_mutex_init(&counter.mutex, nullptr), "pthread_mutex_init");
    pthread_t first, second;
    check_pthread(::pthread_create(&first, nullptr, count_entry, &counter), "create counter 1");
    check_pthread(::pthread_create(&second, nullptr, count_entry, &counter), "create counter 2");
    check_pthread(::pthread_join(first, nullptr), "join counter 1");
    check_pthread(::pthread_join(second, nullptr), "join counter 2");
    std::printf("[main] shared counter=%d (expected 20000)\n", counter.value);
    // 等所有使用者退出且锁处于未锁定状态，才能销毁。destroy 不是解锁操作。
    check_pthread(::pthread_mutex_destroy(&counter.mutex), "pthread_mutex_destroy");
    // 不提供“去掉锁”的可运行版本：并发读写普通 int 是 C++ 数据竞争，行为未定义。
}

struct Mailbox
{
    sem_t empty;
    sem_t full;
    int value = 0;
};

void* producer_entry(void* arg)
{
    Mailbox* mailbox = static_cast<Mailbox*>(arg);
    for (int value = 1; value <= 3; ++value)
    {
        wait_semaphore(&mailbox->empty); // 等到唯一槽位空闲，避免覆盖尚未消费的数据。
        mailbox->value = value;
        std::printf("[producer] put %d\n", value);
        // 发布“有一个数据”，唤醒可能在等待 full 的消费者。
        check_system(::sem_post(&mailbox->full), "sem_post full");
    }
    return nullptr;
}

void* consumer_entry(void* arg)
{
    Mailbox* mailbox = static_cast<Mailbox*>(arg);
    for (int index = 0; index < 3; ++index)
    {
        wait_semaphore(&mailbox->full); // 先等生产者写好，不能提前读槽位。
        std::printf("[consumer] got %d\n", mailbox->value);
        check_system(::sem_post(&mailbox->empty), "sem_post empty");
    }
    return nullptr;
}

void semaphore_example()
{
    std::puts("\n=== 3. semaphore: producer -> single slot -> consumer ===");
    Mailbox mailbox {};
    // sem_init(信号量地址, pshared, 初始计数)：pshared=0 表示同进程线程间共享。
    // 初始有 1 个空槽、0 个数据。这里两个信号量已保证读写交替，无需额外互斥锁。
    // 互斥量强调“谁能进入临界区”；信号量还可表达“资源数量、执行先后”。
    check_system(::sem_init(&mailbox.empty, 0, 1), "sem_init empty");
    check_system(::sem_init(&mailbox.full, 0, 0), "sem_init full");
    pthread_t producer, consumer;
    check_pthread(::pthread_create(&consumer, nullptr, consumer_entry, &mailbox), "create consumer");
    check_pthread(::pthread_create(&producer, nullptr, producer_entry, &mailbox), "create producer");
    check_pthread(::pthread_join(producer, nullptr), "join producer");
    check_pthread(::pthread_join(consumer, nullptr), "join consumer");
    // 不能在线程仍等待或访问某个信号量时销毁它。
    check_system(::sem_destroy(&mailbox.empty), "sem_destroy empty");
    check_system(::sem_destroy(&mailbox.full), "sem_destroy full");
}

void* exit_entry(void* arg)
{
    std::puts("[worker] calling pthread_exit; main remains alive");
    // 参数是交给 join 的返回指针。pthread_exit 不返回，后面的代码不会执行。
    // 不可在这里调用 exit()：exit 或主线程从 main 返回会结束整个进程及全部线程。
    ::pthread_exit(arg);
}

void* cancellable_entry(void* arg)
{
    // 新线程默认允许取消，类型为“延迟取消”：只在取消点响应取消请求。
    // 明确设置一次，方便理解两个 API 的作用；nullptr 表示不保存原设置。
    check_pthread(::pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr), "setcancelstate");
    check_pthread(::pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, nullptr), "setcanceltype");
    check_system(::sem_post(static_cast<sem_t*>(arg)), "notify worker started");
    for (int index = 0; index < 50; ++index)
    {
        ::pthread_testcancel(); // 显式取消点：若有挂起的取消请求，则当前线程在这里结束。
        timespec delay {0, 100000000};
        ::nanosleep(&delay, nullptr); // nanosleep 也是取消点；暂停不是线程同步手段。
    }
    return nullptr;
}

void termination_example()
{
    std::puts("\n=== 4. pthread_exit and deferred pthread_cancel ===");
    int answer = 42; // 属于主线程栈，直到 join 后仍存活，工作线程可以返回它的地址。
    pthread_t thread;
    check_pthread(::pthread_create(&thread, nullptr, exit_entry, &answer), "create exit worker");
    void* result = nullptr;
    check_pthread(::pthread_join(thread, &result), "join exit worker");
    std::printf("[main] pthread_exit returned value=%d\n", *static_cast<int*>(result));

    sem_t started;
    check_system(::sem_init(&started, 0, 0), "sem_init started");
    check_pthread(::pthread_create(&thread, nullptr, cancellable_entry, &started), "create cancel worker");
    wait_semaphore(&started); // 确保工作线程已完成取消设置，不靠 sleep 猜测启动时刻。
    // cancel 只发送取消请求，并不保证返回时对方已经退出；仍需 join 确认和回收。
    check_pthread(::pthread_cancel(thread), "pthread_cancel");
    check_pthread(::pthread_join(thread, &result), "join cancelled worker");
    std::printf("[main] result == PTHREAD_CANCELED: %s\n",
        result == PTHREAD_CANCELED ? "yes" : "no");
    check_system(::sem_destroy(&started), "sem_destroy started");
    // 这个被取消线程不持锁、不管理动态资源。实际业务优先使用“停止标志 + join”
    // 协作退出；取消持锁线程可能造成死锁，应设计清理处理函数并理解取消点。
    // pthread_detach 仅设置结束后自动回收资源，不会停止线程；detach 后不能再 join。
    // 本例需要取得结果并保证参数存活，因此统一使用 join，不使用 detach。
}
} // 匿名命名空间：内部辅助函数只在当前源文件中可见。

void Thread_Test()
{
    // 每阶段完成 join 后再开始下一阶段，便于按日志跟踪；阶段内线程并发运行。
    creation_example();
    mutex_example();
    semaphore_example();
    termination_example();
    std::puts("\n[main] all thread examples completed and joined");
}
