# 定义变量
all : server client

# 最终目标
server : server.o 
	g++ server.o  -o server
client: client.o
	g++ client.o -o client

# 中间文件
server.o: socket.h
client.o: socket.h

# clean
.PHONY: server client
clean:
	-rm server server.o
	-rm client client.o