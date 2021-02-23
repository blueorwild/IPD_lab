#include "./epoll_client.h"

using namespace std;
int main(){
    EpollClient client;
    client.Connect();
    client.HandleEvents();
    return 0;
}