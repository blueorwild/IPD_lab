#ifndef _SOCKET_H_
#define _SOCKET_H_

#include <stdio.h>
#include <stdlib.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

class Socket{
    private:
    int socket_ = -1;
    int connfd_ = -1;

    public:
    Socket(int domain, int type, int protocol){
        // 建立一个协议族为domain、协议类型为type、协议编号为protocol的套接字文件描述符。
        if( (socket_ = socket(domain, type, protocol)) == -1 )
            printf("create socket error!\n");
    }
   ~ Socket(){}

    // 1 成功　-1 失败
    int Bind(sockaddr* addr){
        // 套接字　与套接字绑定的地址　
        if( bind(socket_,  addr, sizeof(*addr)) == -1){
            printf("bind socket error!\n");
            return -1;
        }
        return 1;
    }

    // 1 成功　-1 失败
    int Connect(sockaddr* addr){
        if( connect(socket_, addr, sizeof(*addr)) < 0){
            printf("connect error\n");
            return -1;
        }
        return 1;
    }

    // 1 成功　-1 失败
    int Listen(int backlog){
        // 内核维护的同时等待处理的连接数
        if( listen(socket_, backlog) == -1){
            printf("listen socket error!\n");
            return -1;
        }
        return 1;
    }

    // １ 成功　-1 失败
    int Accept(){
        sockaddr cliaddr; 
        socklen_t len = sizeof(cliaddr);  
        if((connfd_ = accept(socket_, &cliaddr, &len)) == -1){
            printf("accept error!\n");
            return -1;
        }
        return 1;
    }

    // len 是 buffer 大小
    // -1失败，否则实际读的字节数
    int Read(void *buffer, int len){
        int bytes_read = 1;
        int bytes_left = len;
        char *ptr = (char*)buffer;
        while(bytes_read > 0 && bytes_left > 0){
            bytes_read=read(connfd_, ptr, bytes_left);
            if(bytes_read<0){
                printf("read error!\n");
                return(-1);
            }
            bytes_left -= bytes_read;
            ptr += bytes_read;
        }
        return(len - bytes_left);
    }

    // １ 成功　-1 失败
    int Write(const void *buffer,int len){
        int bytes_left = len;
        int write_bytes;
       const char *ptr = (const char *)buffer;
        while(bytes_left>0) {       
            write_bytes = write(socket_, ptr, bytes_left);
            if(write_bytes <=0 ){ 
                printf("write error!\n");
                return(-1);
            }
            bytes_left -= write_bytes;
            ptr += write_bytes;     
        }
        return(1);
    } 

    // 关闭套接字
    void Close(){
        if(connfd_ != -1  && close(connfd_) == -1 ) printf("close  connfd fail!");
        if(socket_ != -1  && close(socket_) == -1 ) printf("close  socket fail!");
    }
};
#endif