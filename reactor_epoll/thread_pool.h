#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#define MAX_THREADS 64
#define MAX_QUEUE 65536

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>


// 错误信号
typedef enum {
    threadpool_invalid        = -1,
    threadpool_lock_failure   = -2,
    threadpool_queue_full     = -3,
    threadpool_shutdown       = -4,
    threadpool_thread_failure = -5
} threadpool_error_t;

// 关闭方式
typedef enum {
    immediate_shutdown = 1,
    graceful_shutdown  = 2
} threadpool_shutdown_t;

// 待执行的任务（函数＋参数）
typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

// 线程池结构
struct threadpool_t {
    pthread_mutex_t lock;                   // 互斥锁
    pthread_cond_t notify;                  // 条件变量
    pthread_t *threads;                         // 线程数组的起始指针
    threadpool_task_t *queue;          // 任务队列数组的起始指针
    int thread_count;                              // 线程数量
    int queue_size;                                   // 任务队列长度
    int head;                                                // 当前任务队列头
    int tail;                                                    // 当前任务队列尾
    int count;                                               // 当前待运行的任务数
    int shutdown;                                      // 线程池当前状态是否关闭
    int started;                                            // 当前活跃的线程数
};

class ThreadPool{
private:
    threadpool_t *pool_;

    // 释放线程池所申请的内存资源
    int threadpool_free(){
        if(pool_ == NULL || pool_->started > 0) {
            return -1;
        }

        if(pool_->threads) {
            free(pool_->threads);
            free(pool_->queue);

            pthread_mutex_lock(&(pool_->lock));
            pthread_mutex_destroy(&(pool_->lock));
            pthread_cond_destroy(&(pool_->notify));
        }
        free(pool_);    
        return 0;
    }


public:
    ThreadPool(){};
    ~ThreadPool(){};

    // 创建线程池
    // 参数：线程数，任务队列长度
    int threadpool_create(int thread_count, int queue_size){
        if(thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)  return -1;
        if((pool_ = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) return -1;

        // 初始化
        pool_->thread_count = 0;
        pool_->queue_size = queue_size;
        pool_->head = pool_->tail = pool_->count = 0;
        pool_->shutdown = pool_->started = 0;

        // 分配线程和任务队列空间
        pool_->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
        pool_->queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t) * queue_size);

        // 初始化互斥锁和条件变量
        if((pool_->threads == NULL) || (pool_->queue == NULL) || 
            (pthread_mutex_init(&(pool_->lock), NULL) != 0) ||
            (pthread_cond_init(&(pool_->notify), NULL) != 0) ) {
            threadpool_free();
            return -1;
        }

        // 创建工作线程
        for(int i = 0; i < thread_count; i++) {
            if(pthread_create(&(pool_->threads[i]), NULL, threadpool_thread, (void*)pool_) != 0) {
                threadpool_destroy(0);
                return -1;
            }
            pool_->thread_count++;
            pool_->started++;
        }

        return 0;
    }

    // 添加需要执行的任务。
    // 参数：任务函数指针，对应函数参数
    int threadpool_add(void (*function)(void *), void *argument){

        if(pool_ == NULL || function == NULL)  return threadpool_invalid;
        // 取得互斥锁
        if(pthread_mutex_lock(&(pool_->lock)) != 0)   return threadpool_lock_failure;

        // 计算下一个可以放task的位置
        int next = (pool_->tail + 1) % pool_->queue_size;

        // 任务队列是否满
        if(pool_->count == pool_->queue_size)   {
            if(pthread_mutex_unlock(&(pool_->lock)) != 0)   return threadpool_lock_failure;
            return threadpool_queue_full;
        }

        // 线程池状态关闭
        if(pool_->shutdown) {
            if(pthread_mutex_unlock(&(pool_->lock)) != 0)   return threadpool_lock_failure;
            return threadpool_shutdown;
        }

        // 在 tail 的位置放置函数指针和参数，添加到任务队列
        pool_->queue[pool_->tail].function = function;
        pool_->queue[pool_->tail].argument = argument;
        pool_->tail = next;
        pool_->count += 1;

        //  发出 signal,表示有 task 被添加进来了
        // 如果有因为任务队列空阻塞的线程，此时会有一个被唤醒
        if(pthread_cond_signal(&(pool_->notify)) != 0) {
            if(pthread_mutex_unlock(&(pool_->lock)) != 0)   return threadpool_lock_failure;
            return threadpool_lock_failure;
        }

        // 释放互斥锁
        if(pthread_mutex_unlock(&(pool_->lock)) != 0)   return threadpool_lock_failure;
        return 0;
    }

    // 销毁存在的线程池。flags 可以指定是立刻结束还是平和结束。
    // 立刻结束指不管任务队列是否为空，立刻结束。平和结束指等待任务队列的任务全部执行完后再结束，在这个过程中不可以添加新的任务。
    int threadpool_destroy(int flags){
        int i, err = 0;

        if(pool_ == NULL)  return threadpool_invalid;

        if(pthread_mutex_lock(&(pool_->lock)) != 0) return threadpool_lock_failure;

        do {
            //  判断是否已在其他地方关闭
            if(pool_->shutdown) {
                err = threadpool_shutdown;
                break;
            }
            
            /* 获取指定的关闭方式 */
            pool_->shutdown = (flags) ? graceful_shutdown : immediate_shutdown;

            // 唤醒所有因条件变量阻塞的线程，并释放互斥锁 
            if((pthread_cond_broadcast(&(pool_->notify)) != 0) ||
            (pthread_mutex_unlock(&(pool_->lock)) != 0)) {
                err = threadpool_lock_failure;
                break;
            }

            // 等待所有线程结束
            for(i = 0; i < pool_->thread_count; i++) {
                if(pthread_join(pool_->threads[i], NULL) != 0) {
                    err = threadpool_thread_failure;
                }
            }
        } while(0);

        /* Only if everything went well do we deallocate the pool */
        if(!err) {
            threadpool_free();
        }
        return err;
    }

    // 线程池每个线程所执行的函数。
    static void *threadpool_thread(void *threadpool) {
        threadpool_t *pool = (threadpool_t *)threadpool;
        threadpool_task_t task;

        while(1){
            // 先拿到锁
            pthread_mutex_lock(&(pool->lock));

            // 等待条件变量，检查是否虚假唤醒。
            // 从pthread_cond_wait()返回时，即拥有锁.
            while((pool->count == 0) && (!pool->shutdown)) {
                pthread_cond_wait(&(pool->notify), &(pool->lock));
            }

            if((pool->shutdown == immediate_shutdown) ||
            ((pool->shutdown == graceful_shutdown) &&
                (pool->count == 0))) {
                break;
            }

            // 取得任务队列的第一个任务
            task.function = pool->queue[pool->head].function;
            task.argument = pool->queue[pool->head].argument;
            pool->head = (pool->head + 1) % pool->queue_size;
            pool->count -= 1;

            // 解锁
            pthread_mutex_unlock(&(pool->lock));

            // 执行任务
            (*(task.function))(task.argument);
        }

        pool->started--;

        pthread_mutex_unlock(&(pool->lock));
        pthread_exit(NULL);
        return(NULL);
    }
};

#endif