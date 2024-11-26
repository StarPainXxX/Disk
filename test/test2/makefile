SRCS:=$(wildcard *.c)  
OBJS:=$(patsubst %.c,%.o,$(SRCS))
CC:=gcc
server: $(OUT)
server:$(OBJS)
	$(CC) $^ -o $@ -lpthread -lcrypt -lmysqlclient

%.o:%.c
	$(CC) -c $^ -o $@ -g

.PHONY:clean rebuild all
clean:
	$(RM) $(OBJS) md5
rebuild: clean all