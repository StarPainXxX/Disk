SRCS:=$(wildcard *.c)
EXES:=$(patsubst %.c,%,$(SRCS))
CC:=gcc
all:$(EXES)
%:%.c
	$(CC) $^ -o $@ -g -lpthread -lmysqlclient
.PHONY:clean rebuild
clean:
	$(RM) $(EXES) 
rebuild:clean all

