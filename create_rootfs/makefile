BUILD_DIR = ./build
OUT = create


CC = gcc

SRCS = $(wildcard *.c)
OBJS = $(wildcard *.o)

all:
	$(CC) $(SRCS) -o $(OUT)

.PHONY: clean

clean: 
	rm $(OBJS) 

