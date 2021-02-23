# 定义变量
objects = main.o server.o

# 最终目标
server: $(objects)
	g++ $(objects) -o server

# 中间文件
main.o: server.h
server.o: server.h

# clean
.PHONY: server
clean:
	-rm server $(objects)