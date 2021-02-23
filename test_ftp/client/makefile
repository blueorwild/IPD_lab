# 定义变量
objects = main.o client.o

# 最终目标
client: $(objects)
	g++ $(objects) -o client

# 中间文件
main.o: client.h
client.o: client.h

# clean
.PHONY: client
clean:
	-rm client $(objects)