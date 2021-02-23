# 定义变量
all : test_timer

# 最终目标
test_timer : test_timer.o my_timer.o
	g++ my_timer.o test_timer.o  -o test_timer -lpthread

# 中间文件
my_timer: my_timer.h

# clean
.PHONY: test_timer
clean:
	-rm test_timer my_timer.o test_timer.o