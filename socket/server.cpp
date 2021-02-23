#include "socket.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <memory.h>

int main(){
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, "127.0.0.1", &addr. sin_addr);
    addr.sin_port = htons(8888);

    char buf[1024] = {'\0'};

    Socket  server(AF_INET, SOCK_STREAM, 0);
    if(server.Bind((sockaddr*)&addr) == 1){
        printf("bind success\n");
        if(server.Listen(10) == 1){
            printf("listen success\n");
            if(server.Accept() == 1){
                printf("accept success\n");
                if(server.Read(buf, 1023) > 0) 
                    printf("read: %s\n", buf);
            }
        }
    }
    
    server.Close();
    return 0;
}