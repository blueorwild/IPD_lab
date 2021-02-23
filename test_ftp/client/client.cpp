#include <iostream>
#include "./client.h"

using std::cout;
using std::endl;

FtpClient::FtpClient(){
	epoll_fd_ = epoll_create(epoll_size);
	connect_fd_= socket(AF_INET, SOCK_STREAM, 0);
	SetNoBlock(connect_fd_);
	Connect();

	// 添加标准输入的事件(非阻塞et模式)
	AddEvent(STDIN_FILENO, EPOLLIN | EPOLLET);
	SetNoBlock(STDIN_FILENO);
}
FtpClient::~FtpClient(){
	close(connect_fd_);
	close(epoll_fd_);
}

void FtpClient::SetNoBlock(int fd){
	int flag;
	if ((flag = fcntl(fd, F_GETFL, NULL)) <  0) 
		cout << "GETFL失败" << endl;
	else if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) == -1)  
		cout << "SETFL失败" << endl;
}

void FtpClient::Connect(){
	// 获取服务器地址
	sockaddr_in  server_addr;
	bzero(&server_addr,  sizeof(server_addr));   // 归零
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port_);
	inet_pton(AF_INET,  server_ip_,  &server_addr.sin_addr);

	// 连接
	int retry_time = 3;
	while(connect(connect_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0 && --retry_time != -1)
		cout << "连接失败" << endl;
	if(retry_time != -1) cout << "连接成功" << endl;
}

void FtpClient::HandleEvents(){
	epoll_event events[epoll_size];     // 存放epoll收到的事件
	char* buf = new char[buf_size_];
	while (1) {
		int readynum = epoll_wait(epoll_fd_,  events,  epoll_size,  -1);     // 阻塞直到有事件发生
		for (int i = 0; i < readynum; i++) {     //开始遍历处理准备好的事务
			int fd = events[i].data.fd;
			if (events[i].events & EPOLLIN)  HandleRead(fd, buf);
			else if (events[i].events & EPOLLOUT) HandleWrite(fd, buf);
		}
	}
}

void FtpClient::HandleRead(int event_fd, char* buf){
	// 来自标准输入的指令(假定不会超过buf_size_)
	if(event_fd == STDIN_FILENO){ 
		memset(buf, '\0', buf_size_);
		int size = read(event_fd, buf, buf_size_);
		if(size <= 0 ) cout << "读取标准输入出错"  << endl;
		else{
			// 排除全是空格或换行
			bool is_null = true;
			for(int i = strlen(buf) - 1; i >=0; --i) 
				if(buf[i] == ' ' || buf[i] == '\n') continue;
				else is_null = false;
			if (is_null) return;
			if(CheckCommand(buf)) SendCommand(buf);
			else cout << "输入指令错误" << endl;
			while(getchar() !=EOF);  // 清空输入缓冲
		}
	}
	// 来自服务器的东西
	else if(event_fd == connect_fd_){
		// 先尝试保存文件名(有可能是Get指令)
		char file_path[256] = {0};
		strcpy(file_path, dir_);
		strcat(file_path, buf + 1);

		memset(buf, '\0', buf_size_);
		int size = read(event_fd, buf, buf_size_);
		if(size <= 0 ) cout << "读取标准输入出错"  << endl;
		else{
			if(buf[0] == CMD_LS) {
				cout << buf+1 << endl;
				DeleteEvent(event_fd, EPOLLIN | EPOLLET);
			}
			else if(buf[0] == CMD_GET){
				FILE* fp = fopen(file_path, "wb");
				fwrite(buf+1, 1, size-1, fp);
				while((size = read(event_fd, buf, buf_size_)) > 0){
					fwrite(buf+1, 1, size-1, fp);
					memset(buf, '\0', buf_size_);
				}
				fclose(fp);
				DeleteEvent(event_fd, EPOLLIN | EPOLLET);
			}
		}
	}
	

}

bool FtpClient::CheckCommand(char* cmd){
	// 需要考虑空格和换行符
	char* point = NULL; 
	char* tmp = cmd; 
	// ls指令－－－－ls－－－－－－－
	if((point = strstr(cmd, "ls")) != NULL){  
		while(tmp != point) {
			if(*tmp != ' ') return false;    // 指令前应该全是空格
			++tmp;
		}
		tmp += 1;    // 跳过指令(只+1是因为下行++)
		while(*++tmp != '\0')    // 指令后应是空格或换行符
			if(*tmp != ' ' && *tmp != '\n') return false;  
		// 规范化指令   l
		memset(cmd, '\0', buf_size_);
		cmd[0] = CMD_LS;
		return true;
	}
	// get指令－－－－get filename－－－－－－－
	else if((point = strstr(cmd, "get")) != NULL){ 
		while(tmp != point) {
			if(*tmp != ' ') return false;    // 指令前应该全是空格
			++tmp;
		}
		tmp += 3;
		if(*tmp != ' ')  return false;   // get filename之间应该是空格
		while(*(++tmp) != '\0')  
			if(*tmp == ' ') continue;    
			else break;
		char filename[256] = {0};
		for(int i = 0; *tmp != '\0' , *tmp != '\n'; ++tmp, ++i)   // 之后的全视作文件名
			filename[i] = *tmp;
		// 规范化指令   g filename
		memset(cmd, '\0', buf_size_);
		cmd[0] = CMD_GET;
		strcpy(cmd + 1, filename);
		return true;
	}
	// put指令－－－－put filename－－－－－－－
	else if((point = strstr(cmd, "put")) != NULL){ 
		while(tmp != point) {
			if(*tmp != ' ') return false;    // 指令前应该全是空格
			++tmp;
		}
		tmp += 3;
		if(*tmp != ' ')  return false;// put filename之间应该是空格
		while(*(++tmp) != '\0')  
			if(*tmp == ' ') continue;    
			else break;
		char filename[256] = {0};
		for(int i = 0; *tmp != '\0' , *tmp != '\n'; ++tmp, ++i)   // 之后的全视作文件名
			filename[i] = *tmp;
		// 规范化指令   p filename
		memset(cmd, '\0', buf_size_);
		cmd[0] = CMD_PUT;
		strcpy(cmd + 1, filename);
		return true;
	}
	else return false;
}
void FtpClient::SendCommand(char* cmd){
	// 根据不同指令做不同操作
	if(cmd[0] == CMD_LS)  // ls指令
		AddEvent(connect_fd_, EPOLLOUT | EPOLLET);
	else if(cmd[0] == CMD_GET)    // get 指令
		AddEvent(connect_fd_, EPOLLOUT | EPOLLET);
	else if(cmd[0] == CMD_PUT){    // put 指令
		// 检查文件是否存在
		char file_path[256] = {0};
		strcpy(file_path, dir_);
		strcat(file_path, cmd + 1);
		if(access(file_path, F_OK) != 0) cout << "文件不存在" << endl;
		else AddEvent(connect_fd_, EPOLLOUT | EPOLLET);
	}
	else cout << "输入指令有误" << endl;
}

void FtpClient::HandleWrite(int event_fd, char* buf){
	if(buf[0] == CMD_LS){
		// 写指令然后事件转为接收内容
		int size = write(event_fd, buf, 2);
		if(size != 2 ) {
			cout << "发送ls指令出错" << endl;
			DeleteEvent(event_fd, EPOLLOUT | EPOLLET);
		}
		else  ChangeEvent(event_fd, EPOLLIN | EPOLLET);
	}
	else if(buf[0] == CMD_GET){
		// 写指令然后事件转为接收内容
		int size = write(event_fd, buf,  buf_size_);
		if(size <=0  ) {
			cout << "发送get指令出错" << endl;
			DeleteEvent(event_fd, EPOLLOUT | EPOLLET);
		}
		else  ChangeEvent(event_fd, EPOLLIN | EPOLLET);
	}
	else if(buf[0] == CMD_PUT){
		// 循环发送(先发送文件名)
		int size = write(event_fd, buf, buf_size_);
		if(size <=0  ) {
			cout << "发送put指令出错" << endl;
			DeleteEvent(event_fd, EPOLLOUT | EPOLLET);
		}
		else{
			char file_path[256] = {0};
			strcpy(file_path, dir_);
			strcat(file_path, buf + 1);
			FILE* fp = fopen(file_path, "rb");
			memset(buf, '\0', buf_size_ );
			int len = 0;
			while((len = fread(buf, 1, buf_size_ , fp)) > 0){
				size = write(event_fd, buf , len);
				if(size != len) {
					cout << "发送put内容出错" << endl;
					break;
				}
				memset(buf, '\0', buf_size_ );
			}
			fclose(fp);
			DeleteEvent(event_fd, EPOLLOUT | EPOLLET); 
		}
	}
}

void FtpClient::AddEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_ADD,  event_fd,  &ev);
}
void FtpClient::DeleteEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_DEL,  event_fd,  &ev);
}
void FtpClient::ChangeEvent(int event_fd, int type){
	epoll_event ev;
	ev.events = type;
	ev.data.fd = event_fd;
	epoll_ctl(epoll_fd_, EPOLL_CTL_MOD,  event_fd,  &ev);
}