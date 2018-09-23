CC=g++
CFAGS= -Wall -std=c++1z -lcrypto -pthread
DEPS = download seed makeMtorrent

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $<

all: client tracker

client: client.cpp makeMtorrent.cpp download.cpp seed.cpp
	$(CC) -o client client.cpp makeMtorrent.cpp download.cpp seed.cpp -lcrypto -pthread

tracker: tracker.cpp
	$(CC) -o tracker tracker.cpp -pthread
