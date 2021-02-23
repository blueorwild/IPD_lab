# 定义变量
all : test_client test_server

# 最终目标
test_client : test_client.o  epoll_client.o
	g++ epoll_client.o test_client.o  -o test_client
test_server : test_server.o  epoll_server.o
	g++ epoll_server.o test_server.o  -o test_server -lpthread

# 中间文件
epoll_client.o: epoll_client.h 
epoll_server.o: epoll_server.h
test_server.o: thread_pool.h

# clean
.PHONY: test_client test_server
clean:
	-rm test_client test_client.o epoll_client.o
	-rm test_server test_server.o epoll_server.o