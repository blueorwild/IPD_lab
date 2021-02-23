#include "./epoll_server.h"

using namespace std;
int main(){
    EpollServer server;
    server.Bind();
    server.HandleEvents();
    return 0;
}