
all:
	g++ -Wall -c util.cpp
	g++ -Wall -g client.cpp util.o -lpthread -o cliente
	g++ -Wall -g peer.cpp util.o -lpthread -o peer

clean:
	rm util.o cliente peer
