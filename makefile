client.o: server.o
	g++ -Wall -std=c++11  -o  client  client.cpp
server.o: 
	g++ -Wall -std=c++11 -Wno-sign-compare  -o  server  server.cpp
clean:
	rm -rf server
	rm -rf client
