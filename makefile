# 编译器改为 g++（C++ 编译器）
CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lpthread

all: server client

server: server.o
	$(CC) $(CFLAGS) server.o -o server $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) client.o -o client $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o server client