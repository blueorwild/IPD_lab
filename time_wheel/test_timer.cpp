#include "my_timer.h"

void* test(void* arg){
    printf("执行到我了，我的定时为：%dms\n", *(int*)arg);
}

int main(){
    MyTimer  timer;

    int time = 1000;   // 1000ms 定时10个滴答
    TaskNode* task = new TaskNode;
    task->func = test;
    task->arg = (void*)(&time);
    timer.AddTask(task, time);

    sleep(30);
    return 0;
}