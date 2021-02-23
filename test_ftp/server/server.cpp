#include "./server.h"

FtpServer::FtpServer(){
	clients_buf_ = new map<int, char*>;

	epoll_fd_ = epoll_create(epoll_size_);
	listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	SetNoBlock(listen_fd_);
	Bind();
	listen(listen_fd_, BACKLOG);
	AddEvent(listen_fd_, EPOLLIN | EPOLLET);  // et模式
}
FtpServer::~FtpServer(){
	close(listen_fd_);
	close(epoll_fd_);
	for(auto&i : *clients_buf_) delete i.second;
	delete clients_buf_;
}

void FtpServer::HandleEvents(){
	epoll_event events[epoll_size_];  // 存储epoll收到的事件
	while (1){
		//获取已经准备好的描述符事件
		int num = epoll_wait(epoll_fd_, events, epoll_size_, -1);
		for (int i = 0; i < num; i++){
			int fd = events[i].data.fd;
			//根据描述符的类型和事件类型进行处理
			if (fd == listen_fd_) HandleAccept();
			else if (events[i].events & EPOLLIN) HandleRead(fd);
			else if (events[i].events & EPOLLOUT) HandleWrite(fd);
		}
	}
}

void FtpServer::Bind(){
	// 规范化地址
	sockaddr_in serve_addr;
	bzero(&serve_addr, sizeof(serve_addr));
	serve_addr.sin_family = AF_INET;
	inet_pton(AF_INET, ip_, &serve_addr.sin_addr);
	serve_addr.sin_port = htons(port_);
	// 绑定
	if (bind(listen_fd_, (struct sockaddr*)&serve_addr, sizeof(serve_addr)) == -1) cout << "绑定端口失败" << endl;
	else cout << "绑定端口成功" << endl;
}

void FtpServer::HandleAccept(){
	sockaddr_in client_addr;
	socklen_t  addr_len = sizeof(client_addr);
	// 非阻塞et模式需要循环接收所有连接，防止并发丢失连接的问题
	int client_fd = 0;
	while ((client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &addr_len)) > 0){
		SetNoBlock(client_fd);
		char* buf = new char[buf_size_];
		(*clients_buf_)[client_fd] = buf;
		AddEvent(client_fd, EPOLLIN | EPOLLET);
		cout << "收到新连接：" << inet_ntoa(client_addr.sin_addr) << ":" <<  client_addr.sin_port << endl;
	}
}

void FtpServer::HandleRead(int event_fd){
	// 若是文件内容，非阻塞ET模式需要一次性读取完
	int size = 0;
	char* buf = (*clients_buf_)[event_fd];
	memset(buf, '\0', buf_size_);
	// 先读一次，根据头部内容判断是什么操作
	if((size = read(event_fd, buf, buf_size_)) > 0 ){
		if(buf[0] == CMD_LS){     // ls指令
			if(ThroughDir(buf)) ChangeEvent(event_fd, EPOLLOUT | EPOLLET);
		}  // 这个大括号有点坑。。
		else if(buf[0] == CMD_GET){  // get指令
			//buf弄成  g file_path 的形式
			char file_path[256] = {0};
			strcpy(file_path, dir_);
			strcat(file_path, buf + 1);
			memset(buf + 1, '\0', buf_size_);
			strcpy(buf + 1, file_path);

			ChangeEvent(event_fd, EPOLLOUT | EPOLLET);
		}
		else if(buf[0] == CMD_PUT){  // put指令
			// 第一个包是文件名
			char file_path[256] = {0};
			strcpy(file_path, dir_);
			strcat(file_path, buf + 1);
			memset(buf , '\0', buf_size_);

			FILE* fp = fopen(file_path, "wb");
			while((size = read(event_fd, buf, buf_size_)) > 0){
				fwrite(buf, 1, size, fp);
				memset(buf , '\0', buf_size_);
			}
			fclose(fp);
		}
	}
	else cout << "读取客户端内容出错或客户端已关闭：" << event_fd << endl; 

}
void FtpServer::HandleWrite(int event_fd){
	int size = 0;
	char* buf = (*clients_buf_)[event_fd];
	if(buf[0] == CMD_LS){
		if((size = write(event_fd, buf , buf_size_ )) <= 0)  cout << "发送ls内容出错" << endl;
		ChangeEvent(event_fd, EPOLLIN | EPOLLET);
	}
	else if(buf[0] == CMD_GET){
		if(access(buf+1, F_OK) == 0){
			FILE* fp = fopen(buf + 1, "rb");
			memset(buf + 1, '\0', buf_size_ - 1);
			int len = 0;
			while((len = fread(buf+1, 1, buf_size_-1, fp)) > 0){
				size = write(event_fd, buf , len + 1);
				if(size != len + 1) {
					cout << "发送get内容出错" << endl;
					break;
				}
				memset(buf + 1, '\0', buf_size_ - 1);
			}
			fclose(fp);
		}
		ChangeEvent(event_fd, EPOLLIN | EPOLLET);
	}
}

void FtpServer::AddEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_ADD,  event_fd,  &ev);
}
void FtpServer::DeleteEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL,  event_fd,  &ev);
}
void FtpServer::ChangeEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_MOD,  event_fd,  &ev);
}

void FtpServer::SetNoBlock(int fd){
	int flag;
	if ((flag = fcntl(fd, F_GETFL, NULL)) <  0) 
		cout << "GETFL失败" << endl;
	else if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)  
		cout << "SETFL失败" << endl;
}

bool FtpServer::ThroughDir(char* buf){
	// 最终形成  l file_name1 \n filename2 \n …… 的形式
	memset(buf, '\0', buf_size_);
	buf[0] = CMD_LS;
	char* tmp = buf + 1;

	DIR* dir;
	dirent* dir_ptr;
	if((dir = opendir(dir_)) == NULL) cout << "打开文件夹出错" << endl;
	else{
		while((dir_ptr = readdir(dir)) != NULL){
			if(strcmp(dir_ptr->d_name,".") == 0 || strcmp(dir_ptr->d_name,"..") == 0)   // 当前文件夹和父文件夹
				continue;
        	else if(dir_ptr->d_type == DT_REG){   // 常规文件
				int len = strlen(dir_ptr->d_name);
				strncpy(tmp, dir_ptr->d_name, len);
				tmp += len;
				*(tmp++) = '\n';    // 每一个文件名后添加换行符
			}
		}
		closedir(dir);
	}
	if(strlen(buf) == 1) return false;
	return true;
}