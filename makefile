CC = g++
CFLAGS = -Wall -g
LDFLAGS = -lpthread

# all: server client
all: server client manage_users

server: server.o
	$(CC) $(CFLAGS) server.o -o server $(LDFLAGS)

client: client.o
	$(CC) $(CFLAGS) client.o -o client $(LDFLAGS)


manage_users: manage_users.o
	$(CC) $(CFLAGS) manage_users.o -o manage_users

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f *.o server client

clean:
	rm -f *.o server client manage_users