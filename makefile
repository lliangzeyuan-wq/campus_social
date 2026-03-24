CXXFLAGS = -Wall -std=c++11
LDFLAGS = -lmysqlclient -lpthread

all: server client manage_users

server: server.cpp common.h db.h db.cpp
	g++ $(CXXFLAGS) -o server server.cpp db.cpp $(LDFLAGS)

client: client.cpp common.h
	g++ $(CXXFLAGS) -o client client.cpp $(LDFLAGS)

manage_users: manage_users.cpp common.h db.h db.cpp
	g++ $(CXXFLAGS) -o manage_users manage_users.cpp db.cpp $(LDFLAGS)

clean:
	rm -f server client manage_users