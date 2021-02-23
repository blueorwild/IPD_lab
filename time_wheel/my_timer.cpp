#include "my_timer.h"

void MyTimer::DoBegin(){
    // 启动定时器
    pthread_t t;
        pthread_create(&t, NULL, Begin, this);
};
void* MyTimer::Begin(void * arg){
    MyTimer* timer = (MyTimer*) arg;
    while(1){
        usleep(1000 * TIMER_INTERVAL);   // 一个滴答
        // 更新滴答计数
        int cur_dida;
        pthread_mutex_lock(&(timer->lock_));
        cur_dida = ++timer->dida_;
        printf("当前定时器滴答计数为：%d \n", cur_dida);
        if(timer->dida_ == TIMER_SIZE) {
            pthread_mutex_lock(&(timer->lock_));
                printf("定时器已结束\n");
            timer->timer_is_ok_ = false;
            return NULL;
        }
        pthread_mutex_unlock(&(timer->lock_));

        //  然后执行对应节点的任务
        if(timer->timer_[cur_dida].size() > 0){
            for(auto&i : timer->timer_[cur_dida]){
                pthread_t t;
                pthread_create(&t, NULL, i->func, i->arg);
            }
            // 然后删除节点
            timer->timer_[cur_dida].empty();
        } 
    }
}

MyTimer::MyTimer(){
    dida_ = 0;
    timer_.resize(TIMER_SIZE);
    DoBegin();
}
MyTimer::~MyTimer(){
    // 清理任务节点内存
    for(auto&i : timer_)
        if(i.size() > 0 )
            for(auto &j : i) delete j;
}

//  添加一个定时任务
// 任务结构　定时时间ms（从现在开始）
void MyTimer::AddTask(TaskNode* task, int time_ms){
    // 计算该定时任务应该放在哪里
    int count = time_ms / TIMER_INTERVAL;
    pthread_mutex_lock(&lock_);
    count += dida_;
    pthread_mutex_unlock(&lock_);

    if(count >= TIMER_SIZE)  printf("添加定时任务失败：超过定时器最大时间！\n");
    else timer_[count].push_front(task);
}
