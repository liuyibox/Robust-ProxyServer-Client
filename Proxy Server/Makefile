COMPILER = g++
CFLAGS = -g -c -Wall

all:
	$(COMPILER) $(CFLAGS) HttpProxy.cpp -o http.o
	$(COMPILER) $(CFLAGS) ProxyServer.cpp -o server.o
	$(COMPILER) $(CFLAGS) ProxyClient.cpp -o client.o
	$(COMPILER) -g client.o http.o -o client
	$(COMPILER) -g server.o http.o -o server

clean:
	rm *.o client server
