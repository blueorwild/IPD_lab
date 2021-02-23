#ifndef _MY_TIMER_H_
#define _MY_TIMER_H_

#define  TIMER_INTERVAL    100     // 每一个滴答100ms
#define  TIMER_SIZE       18   // 此定时器总共18个格子　即定时器最大支持1.8s内的定时任务

#include <list>
#include <vector>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

using std::list;
using std::vector;

// 时间轮的每个格子的任务链表节点
struct TaskNode{
    void* (*func)(void *arg);    // 函数指针
    void *arg;    // 参数
};

class MyTimer{
private:
    int dida_;                 // 当前时钟滴答计数
    bool timer_is_ok_;    // 定时器是否可用（超过最大时间即不可用）
    pthread_mutex_t lock_;                   // 用于时钟滴答计数的互斥锁
    vector<list<TaskNode*>> timer_;   // 定时器时间轮盘

    // 定时器初始化后即开始计时（线程
    void DoBegin();
    static void* Begin(void * arg);

public:
    MyTimer();
    ~MyTimer();

    //  添加一个定时任务
    // 任务结构　定时时间ms（从现在开始）
    void AddTask(TaskNode* task, int time_ms);

};

#endif