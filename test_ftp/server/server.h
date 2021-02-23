#ifndef _FTP_SERVER_H_
#define  _FTP_SERVER_H_

#define CMD_LS 'l'
#define CMD_GET 'g'
#define CMD_PUT 'p'
#define BACKLOG 100   // TCP SYN RECEIVED和ESTABLISHED状态的套接字的队列大小

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <map>
#include <iostream>

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>


using std::cout;
using std::endl;
using std::map;

class FtpServer{
private:
    // 监听的地址端口
    const char* ip_ = "127.0.0.1";
    const int port_ = 6666;

    const int epoll_size_ = 1024;      // epoll最大监听的事件数目
    int epoll_fd_ = 0;                              // epoll句柄
    int listen_fd_ = 0;                             // 监听句柄
    map<int, char*> *clients_buf_;              // 客户端fd->客户端的buf
    const int buf_size_ = 1024;           // 通信过程中buf大小,首位标志位
    const char* dir_ = "./data/";           // 数据文件目录

    // 创建套接字并绑定
    void Bind();

    // 添加/删除/修改 一个epoll监听事件
    void AddEvent(int event_fd, int type);
    void DeleteEvent(int event_fd, int type);
    void ChangeEvent(int event_fd, int type);

    // 设置套接字非阻塞
    void SetNoBlock(int fd);
    // 处理连接、读、写事件
    void HandleAccept();
    void HandleRead(int event_fd);
    void HandleWrite(int event_fd);

    // 遍历文件夹获取名字放入buf(假设不会超过buf_size_)
    bool ThroughDir(char* buf);  // 有文件则返回true

public:
    FtpServer();
    ~FtpServer();

    // 处理epoll监听的事件(非阻塞)
    void HandleEvents();
};

#endif