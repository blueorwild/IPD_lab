#ifndef _FTP_SERVER_H_
#define  _FTP_SERVER_H_

#define BACKLOG   20   // TCP SYN RECEIVED和ESTABLISHED状态的套接字的队列大小
#define IP     "127.0.0.1"
#define PORT     8888
#define EPOLL_SIZE     1024   // 最大监听事件数目
#define BUF_SIZE    1024         // 通信过程中buf大小

#define THREAD 16                   // 线程池线程数量
#define QUEUE  64                   // 线程池任务队列数

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <map>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>

#include "thread_pool.h"

using std::map;

class EpollServer{
private:
    int epoll_fd_ = 0;                              // epoll句柄
    int listen_fd_ = 0;                             // 监听句柄
    
    ThreadPool threadpool_;           // 用于reactor的线程池

    // 设置套接字非阻塞
    void SetNoBlock(int fd);
    // 处理连接、读、写事件
    void HandleAccept();
    void HandleRead(int event_fd);
    void HandleWrite(int event_fd);

public:
    map<int, char*> *clients_buf_;              // 客户端fd->客户端的buf

    EpollServer();
    ~EpollServer();

    // 绑定并开始监听
    void Bind();

    // 处理epoll监听的事件(非阻塞)
    void HandleEvents();


    // 添加/删除/修改 一个epoll监听事件
    void AddEvent(int event_fd, int type);
    void DeleteEvent(int event_fd, int type);
    void ChangeEvent(int event_fd, int type);
};

struct TaskArg{
	int event_fd;       // 发生事件的文件句柄
	EpollServer* epoll;    // epoll 用于注册事件
};

#endif