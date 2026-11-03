#pragma once
// POSIX 线程分阶段示例：创建/传参/等待、互斥量、信号量、主动退出与延迟取消。
// Linux 编译及链接时均使用 -pthread；从 main 调用后自动运行全部阶段。
void Thread_Test();
