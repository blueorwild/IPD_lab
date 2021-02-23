
#include "./epoll_client.h"

EpollClient::EpollClient(){
	epoll_fd_ = epoll_create(EPOLL_SIZE);
	connect_fd_= socket(AF_INET, SOCK_STREAM, 0);
	SetNoBlock(connect_fd_);
}
EpollClient::~EpollClient(){
	close(connect_fd_);
	close(epoll_fd_);
}

void EpollClient::SetNoBlock(int fd){
	int flag;
	if ((flag = fcntl(fd, F_GETFL, NULL)) <  0) 
		printf("GETFL失败\n");
	else if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)  
		printf("SETFL失败\n");
}

void EpollClient::Connect(){
	// 获取服务器地址
	sockaddr_in  server_addr;
	bzero(&server_addr,  sizeof(server_addr));   // 归零
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET,  SERVER_IP,  &server_addr.sin_addr);

	// 连接
	int retry_time = 3;
	while(connect(connect_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 && --retry_time != -1){
		printf("连接失败\n");
	}
	if(retry_time != -1) {
		printf("连接成功\n");
		// 添加标准输入的事件(非阻塞et模式)
		AddEvent(STDIN_FILENO, EPOLLIN | EPOLLET);
		SetNoBlock(STDIN_FILENO);
	}
}

void EpollClient::HandleEvents(){
	epoll_event events[EPOLL_SIZE];     // 存放epoll收到的事件
	while (1) {
		int readynum = epoll_wait(epoll_fd_,  events,  EPOLL_SIZE,  -1);     // 阻塞直到有事件发生
		for (int i = 0; i < readynum; i++) {     //开始遍历处理准备好的事务
			int fd = events[i].data.fd;
			if (events[i].events & EPOLLIN)  HandleRead(fd);
			else if (events[i].events & EPOLLOUT) HandleWrite(fd);
		}
	}
}

void EpollClient::HandleRead(int event_fd){
	// 来自标准输入的指令(假定不会超过buf_size_)
	if(event_fd == STDIN_FILENO){ 
		memset(buf_, '\0', BUF_SIZE);
		int size = read(event_fd, buf_, BUF_SIZE);
		if(size <= 0 ) printf("读取标准输入内容出错\n");
		else AddEvent(connect_fd_, EPOLLOUT | EPOLLET);

	}
	// 来自服务器的东西(直接打印)
	else if(event_fd == connect_fd_){
		char buf[BUF_SIZE] = {'\0'};
		int size = read(event_fd, buf, BUF_SIZE);
		if(size <= 0 ) printf("读取服务器内容出错\n");
		else{
			printf("读取服务器内容：%s\n", buf);
			DeleteEvent(event_fd, EPOLLIN | EPOLLET );
		}
	}
}

void EpollClient::HandleWrite(int event_fd){
	int size = write(event_fd, buf_, BUF_SIZE);
	if(size > 0){
		printf("往服务器发送内容：%s\n", buf_);
		ChangeEvent(event_fd, EPOLLIN | EPOLLET );
	}
	else printf("往服务器发送内容出错\n");
}

void EpollClient::AddEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_ADD,  event_fd,  &ev);
}
void EpollClient::DeleteEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL,  event_fd,  &ev);
}
void EpollClient::ChangeEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_MOD,  event_fd,  &ev);
}