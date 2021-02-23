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

    char buf[1024] = "hello world";

    Socket  client(AF_INET, SOCK_STREAM, 0);
    if(client.Connect((sockaddr*)&addr) == 1){
        printf("connect success\n");
        if(client.Write(buf, 1023) == 1)
            printf("write: %s!\n",buf);
    }

    client.Close();
    return 0;
}