build: client/client.cpp include/include.h
	@g++ client/client.cpp -o client/client -pthread -I include
	@g++ server/server.cpp -o server/server -pthread -I include

clean:
	-rm client/client
	-rm server/server