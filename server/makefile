SRCS:=$(wildcard *.c)  
OBJS:=$(patsubst %.c,%.o,$(SRCS))
CC:=gcc
server: $(OUT)
server:$(OBJS)
	$(CC) $^ -o $@ -lpthread

%.o:%.c
	$(CC) -c $^ -o $@ -g

.PHONY:clean rebuild all
clean:
	$(RM) $(OBJS) server
rebuild: clean all
