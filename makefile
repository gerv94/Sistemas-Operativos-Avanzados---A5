CC=g++
run:
	$(CC) -o server.o server/server.cpp -I./Shared
	$(CC) -o client.o client/client.cpp -I./Shared