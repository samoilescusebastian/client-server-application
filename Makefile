CFLAGS = -Wall -g -Wextra


all: server subscriber

server: server.cpp message.cpp client.cpp

subscriber: subscriber.cpp message.cpp client.cpp

.PHONY: clean run_server run_subscriber

run_server:
	./server 

run_subscriber:
	./subscriber

clean:
	rm -f server subscriber