#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <iostream>
#include <vector>

#include "package.hpp"

enum e_status
{
	accept,
	reject,
	messaging,
	pending
};

/**
 * Structure of a client status
 * int id;
 * char name[128];
 * e_status status;
 * char partner[128];
 **/
struct client
{
	int id;
	char name[128];
	e_status status;
	int partner;
};

#define LOG(x) std::cout << "[" << (pid == getpid() ? "Server" : "Pipe") << ": " << getpid() << "] " << x << std::endl
#define OPEN(file, flag, message) \
	open(file, flag);             \
	if (fd == -1)                 \
	{                             \
		LOG(message);             \
		exit(1);                  \
	}

class server
{
private:
	std::vector<client> clients = {};
	std::string server_name;
	const char *pipe = "/tmp/server.pipe";
	int pid;
	const int max_users = 5;

	void createPipe();
	void deletePipe();
	void createChatPipe(char *);
	void runPipe(int);
	void runChat(int, int);

	void getInfo();
	void sendInfo(package::info);
	void sendMessage(package::info);
	int getClientIndex(int);

	void printUsers();

public:
	server(/* args */);
	~server();

	void run();
};