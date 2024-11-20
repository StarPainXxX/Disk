SRCS:=$(wildcard *.c)  
OBJS:=$(patsubst %.c,%.o,$(SRCS))
CC:=gcc
client: $(OUT)
client:$(OBJS)
	$(CC) $^ -o $@ -lpthread

%.o:%.c
	$(CC) -c $^ -o $@ -g

.PHONY:clean rebuild all
clean:
	$(RM) $(OBJS) client
rebuild: clean all

