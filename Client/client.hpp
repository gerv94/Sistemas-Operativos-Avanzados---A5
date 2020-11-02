#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <iostream>

#include "package.hpp"

#define OPEN(file, flag, message) open(file, flag);if (fd == -1){std::cout << message;exit(1);}

class client
{
private:
	/* data */
	int pid;
	char waitPipe[64];
	char concPipe[64];
	bool connected;
	package::info info;
	const char* server_pipe = "/tmp/server.pipe";

	void createPipe();
	void createPipe(char *);
	void deletePipe();

	void runLogged();
	void runConc();
	void runChat(char *);
	void printMenu();

	void getInfo();
	void sendInfo();
public:
	client(/* args */);
	~client();

	void run();
	void sendExit();
};
