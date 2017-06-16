build: server client

server: server.cpp
	g++ -std=c++11 -Wall -g -lnsl server.cpp -o server

client: client.cpp
	g++ -std=c++11 -Wall -g -lnsl client.cpp -o client

clean:
	rm -f server client
