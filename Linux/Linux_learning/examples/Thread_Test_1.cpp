#include "Thread_Test_1.h"
#include <pthread.h>
#include <iostream>
#include <semaphore.h>
struct studentScore
{
	int math;
	int English;
	int Chinese;
	char name[256];
};
struct mutexStruct
{
	pthread_mutex_t mutex;
	sem_t thread1;
	sem_t thread2;
	int count = 0;
};
void* calculateScore(void* arg)
{
	studentScore* student = static_cast<studentScore*>(arg);
	pthread_t thread_id = pthread_self();
	std::printf("pthread id: %lu\n",static_cast<unsigned long>(thread_id));
	printf("%s's score is:%d\n", student->name, student->math + student->English + student->Chinese);
	return nullptr;
}
void* printCount1(void* arg)
{
	pthread_t thread_id = pthread_self();
	mutexStruct* countStruct = static_cast<mutexStruct*>(arg);
	for (int i = 0; i < 10; i++)
	{
		sem_wait(&countStruct->thread1);
		std::printf("pthread id: %lu\n", static_cast<unsigned long>(thread_id));
		pthread_mutex_lock(&countStruct->mutex);
		printf("current count is: %d\n", countStruct->count);
		countStruct->count++;
		pthread_mutex_unlock(&countStruct->mutex);
		sem_post(&countStruct->thread2);
	}
	return nullptr;
}
void* printCount2(void* arg)
{
	pthread_t thread_id = pthread_self();
	mutexStruct* countStruct = static_cast<mutexStruct*>(arg);
	for (int i = 0; i < 10; i++)
	{
		sem_wait(&countStruct->thread2);
		std::printf("pthread id: %lu\n", static_cast<unsigned long>(thread_id));
		printf("current count is: %d\n", countStruct->count);
		pthread_mutex_lock(&countStruct->mutex);
		countStruct->count++;
		pthread_mutex_unlock(&countStruct->mutex);
		sem_post(&countStruct->thread1);
	}
	return nullptr;
}
void myThreadTest()
{
	studentScore arg{100,90,80,"xiaoming"};
	pthread_t thread;
	pthread_create(&thread, nullptr, calculateScore, &arg);
	pthread_join(thread, nullptr);

	mutexStruct arg1{};
	pthread_t thrad1, thread2;
	pthread_mutex_init(&arg1.mutex, NULL);
	sem_init(&arg1.thread1, 0, 1);
	sem_init(&arg1.thread2, 0, 0);
	pthread_create(&thrad1, nullptr, printCount1, &arg1);
	pthread_create(&thread2, nullptr, printCount2, &arg1);
	pthread_join(thrad1, nullptr);
	pthread_join(thread2, nullptr);
	sem_destroy(&arg1.thread1);
	sem_destroy(&arg1.thread2);
	pthread_mutex_destroy(&arg1.mutex);
}