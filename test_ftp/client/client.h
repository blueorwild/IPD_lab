#ifndef _FTP_CLIENT_H_
#define _FTP_CLIENT_H_

#define CMD_LS 'l'
#define CMD_GET 'g'
#define CMD_PUT 'p'

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

class FtpClient{
private:
    // 服务器地址
    const char* server_ip_ = "127.0.0.1";
    const int server_port_ = 6666;

    const int epoll_size = 1024;         // epoll最大监听的事件数目
    int epoll_fd_ = 0;                              // 客户端的epoll句柄
    int connect_fd_ = 0;                        // 用于连接的套接字句柄
    const int buf_size_ = 1024;           // 通信过程中buf大小
    const char* dir_ = "./data/";           // 数据文件目录

    // 与服务器发起连接
    void Connect();

    // 添加/删除/修改 一个epoll监听事件
    void AddEvent(int event_fd, int type);
    void DeleteEvent(int event_fd, int type);
    void ChangeEvent(int event_fd, int type);

    // 设置套接字非阻塞
    void SetNoBlock(int fd);
    // 处理读、写事件
    void HandleRead(int event_fd, char* buf);
    void HandleWrite(int event_fd, char* buf);

    // 检查并规范化指令、发送指令
    bool CheckCommand(char* cmd);  // 若是正确指令返回true
    void SendCommand(char* cmd);

public:
    FtpClient();
    ~FtpClient();

    // 处理epoll监听的事件(非阻塞)
    void HandleEvents();
};

#endif  // !_FTP_CLIENT_H_