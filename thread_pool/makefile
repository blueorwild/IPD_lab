# 定义变量
all : test

# 最终目标
test : test.o  
	g++ test.o  -o test -lpthread

# 中间文件
test.o: thread_pool.h

# clean
.PHONY: test
clean:
	-rm test test.o