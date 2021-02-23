#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_

#define BACKLOG   20   // TCP SYN RECEIVED和ESTABLISHED状态的套接字的队列大小
#define SERVER_IP     "127.0.0.1"
#define SERVER_PORT     8888
#define EPOLL_SIZE     1024   // 最大监听事件数目
#define BUF_SIZE    1024         // 通信过程中buf大小

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>

using std::map;

class EpollClient{
private:
    // 服务器地址
    int epoll_fd_ = 0;                              // 客户端的epoll句柄
    int connect_fd_ = 0;                        // 用于连接的套接字句柄
    char buf_[BUF_SIZE];                    // 用于存放通信数据

    // 添加/删除/修改 一个epoll监听事件
    void AddEvent(int event_fd, int type);
    void DeleteEvent(int event_fd, int type);
    void ChangeEvent(int event_fd, int type);

    // 设置套接字非阻塞
    void SetNoBlock(int fd);
    // 处理读、写事件
    void HandleRead(int event_fd);
    void HandleWrite(int event_fd);

public:
    EpollClient();
    ~EpollClient();

    // 与服务器发起连接
    void Connect();

    // 处理epoll监听的事件(非阻塞)
    void HandleEvents();
};

#endif  // !_FTP_CLIENT_H_