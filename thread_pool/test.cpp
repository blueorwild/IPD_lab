#define THREAD 16
#define QUEUE  64

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

#include "thread_pool.h"

int tasks = 0, done = 0;
pthread_mutex_t lock;

void dummy_task(void *arg) {
    // 休眠是为了等待任务添加完成
    usleep(50000);
    pthread_mutex_lock(&lock);
    // 记录成功完成的任务数
    ++done;
    printf("完成任务%d\n", *(int*)arg);
    pthread_mutex_unlock(&lock);
}

int main(int argc, char **argv){
    ThreadPool  threadpool;

    pthread_mutex_init(&lock, NULL);

    assert((threadpool.threadpool_create(THREAD, QUEUE)) != -1);
    printf("创建线程池：%d个线程 ，任务队列长度%d\n", THREAD, QUEUE);

    // 只要任务队列还没满，就一直添加 
    int num[QUEUE * 2] = {0};
    for(int i =0; i < QUEUE * 2; ++i) num[i] = i;
    int* ptr = num;
    while(threadpool.threadpool_add(&dummy_task, (void*)ptr) == 0) {
        pthread_mutex_lock(&lock);
        tasks++;
        printf("添加任务%d\n", *ptr);
        ++ptr;
        pthread_mutex_unlock(&lock);
    }

    printf("总计添加 %d 个任务\n", tasks);

    // 不断检查任务数是否完成一半以上，没有则继续休眠 
    while((tasks / 2) > done) {  usleep(10000); }

    // 这时候销毁线程池,0 代表 immediate_shutdown
    assert(threadpool.threadpool_destroy(0) == 0);
    printf("完成了 %d 个任务\n", done);

    return 0;
}