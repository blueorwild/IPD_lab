#include "./epoll_server.h"

EpollServer::EpollServer(){
	if((threadpool_.threadpool_create(THREAD, QUEUE)) == -1){
		printf("创建线程池失败\n");
		return;
	}

	clients_buf_ = new map<int, char*>;
	
	if((epoll_fd_ = epoll_create(EPOLL_SIZE)) == -1) {
		printf("创建epoll失败\n");
		return;
	}
}
EpollServer::~EpollServer(){
	close(listen_fd_);
	close(epoll_fd_);
	for(auto&i : *clients_buf_) delete i.second;
	delete clients_buf_;
	threadpool_.threadpool_destroy(0);
}

void EpollServer::Bind(){
	if((listen_fd_ = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		printf("创建监听套接字失败\n");
		return;
	}
	SetNoBlock(listen_fd_);

	// 规范化地址
	sockaddr_in serve_addr;
	bzero(&serve_addr, sizeof(serve_addr));
	serve_addr.sin_family = AF_INET;
	inet_pton(AF_INET, IP, &serve_addr.sin_addr);
	serve_addr.sin_port = htons(PORT);
	
	// 绑定
	if (bind(listen_fd_, (struct sockaddr*)&serve_addr, sizeof(serve_addr)) == -1) printf("绑定端口失败\n");
	else {
		printf("绑定端口成功\n");
		listen(listen_fd_, BACKLOG);
		AddEvent(listen_fd_, EPOLLIN | EPOLLET);  // et模式
	}
}

void ReadTask(void *arg) ;
void WriteTask(void *arg) ;
void EpollServer::HandleEvents(){
	epoll_event events[EPOLL_SIZE];  // 存储epoll收到的事件
	while (1){
		//获取已经准备好的描述符事件
		int num = epoll_wait(epoll_fd_, events, EPOLL_SIZE, -1);
		for (int i = 0; i < num; i++){
			int fd = events[i].data.fd;
			//根据描述符的类型和事件类型进行处理
			if (fd == listen_fd_) HandleAccept();
			else {
				TaskArg * arg  = new TaskArg;
				arg->epoll = this;
				arg->event_fd = fd;
				if (events[i].events & EPOLLIN)   // 读事件
					threadpool_.threadpool_add(ReadTask, arg);
				else if  (events[i].events & EPOLLOUT)  // 写事件
					threadpool_.threadpool_add(WriteTask, arg);
			}
		}
	}
}

void EpollServer::HandleAccept(){
	sockaddr_in client_addr;
	socklen_t  addr_len = sizeof(client_addr);
	// 非阻塞et模式需要循环接收所有连接，防止并发丢失连接的问题
	int client_fd = 0;
	while ((client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &addr_len)) > 0){
		SetNoBlock(client_fd);
		char* buf = new char[BUF_SIZE];
		(*clients_buf_)[client_fd] = buf;
		AddEvent(client_fd, EPOLLIN | EPOLLET);
		printf("收到新连接：%s : %d\n", inet_ntoa(client_addr.sin_addr),  client_addr.sin_port);
	}
}

void EpollServer::AddEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_ADD,  event_fd,  &ev);
}
void EpollServer::DeleteEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL,  event_fd,  &ev);
}
void EpollServer::ChangeEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_MOD,  event_fd,  &ev);
}

void EpollServer::SetNoBlock(int fd){
	int flag;
	if ((flag = fcntl(fd, F_GETFL, NULL)) <  0) 
		printf("GETFL失败\n");
	else if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)  
		printf("SETFL失败\n");
}

void ReadTask(void *arg) {
	TaskArg * real_arg = (TaskArg*)arg;
	int event_fd = real_arg->event_fd;
	EpollServer* epoll =  real_arg->epoll;

	int size = 0;
	char * buf = (*epoll->clients_buf_)[event_fd];
	if((size = read(event_fd, buf, BUF_SIZE)) > 0 ){
		// 正常读到消息则打印并写回
		printf("收到客户端消息：%s\n", buf);
		epoll->ChangeEvent(event_fd, EPOLLOUT | EPOLLET);
	}
	else{
		// 否则打印错误信息并清理内存
		printf("读取客户端内容出错或客户端已关闭：%d\n", event_fd);
		delete real_arg;
	}
}

void WriteTask(void *arg) {
	TaskArg * real_arg = (TaskArg*)arg;
	int event_fd = real_arg->event_fd;
	EpollServer* epoll =  real_arg->epoll;
	char* buf = (*epoll->clients_buf_)[event_fd];

	int size = 0;
	if((size = write(event_fd, buf , BUF_SIZE )) > 0)  {
		// 正常写了则又换成读
		printf("往客户端写内容：%s\n", buf);
		epoll->ChangeEvent(event_fd, EPOLLIN | EPOLLET);
	}
	else printf("往客户端写内容出错或断开连接\n");
	delete real_arg;
}
