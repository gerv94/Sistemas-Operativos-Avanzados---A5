#include "client.hpp"

client::client(/* args */)
{
	pid = getpid();
	info.id = pid;
	sprintf(waitPipe, "/tmp/wait%d.pipe", pid); // pipe de espera
	sprintf(concPipe, "/tmp/conc%d.pipe", pid); // pipe de espera
	connected = false;
}

client::~client()
{
	deletePipe();
	std::cout << "Bye!\n";
}

void client::run()
{
	int fd;
	std::cout << "Welcome to chat v0.1!\n";
	createPipe();

	// Login
	info.action = package::login;
	std::cout << "Enter your name to login:\n";
	std::cin.getline(info.content, sizeof(package::info::content));

	fd = OPEN(server_pipe, O_WRONLY, "Sorry the server is down :(\n");
	write(fd, &info, sizeof(package::info));
	close(fd);

	// Get response
	fd = OPEN(waitPipe, O_RDONLY, "Sorry the server is down :(\n");
	read(fd, &info, sizeof(package::info));
	close(fd);

	if (info.ok)
	{
		connected = true;
		std::cout << info.content << "\n";
		runConc();
		runLogged();
	}
	else
	{
		std::cout << info.content << "\n";
	}
}

void client::runLogged()
{
	char command[64];
	printMenu();
	while (true)
	{
		std::cout << "\nEnter a command:\n";
		std::cin >> command;
		//std::cin.getline(command, sizeof(command));

		if (strcmp(command, "/help") == 0)
		{
			printMenu();
		}
		else if (strcmp(command, "/list") == 0)
		{
			// Get response
			info.action = package::list;
			sprintf(info.content, "list");
			sendInfo();

			getInfo();
			std::cout << info.content << "\n";
		}
		else if (strcmp(command, "/chat") == 0)
		{
			std::cin >> command;
			std::cout << "Waiting for " << command << " to accept this chat\n";

			info.action = package::connect;
			sprintf(info.content, command);
			sendInfo();

			getInfo();
			if (info.ok)
			{
				runChat(info.content);
			}
			else
			{
				std::cout << info.content << "\n";
			}
		}
		else if (strcmp(command, "/check") == 0)
		{
			info.action = package::check;
			sprintf(info.content, "check");
			sendInfo();

			getInfo();

			std::cout << info.content << "\n";
		}
		else if (strcmp(command, "/accept") == 0)
		{
			info.action = package::accept;
			sprintf(info.content, "accept");
			sendInfo();
			getInfo();
			if (info.ok)
			{
				runChat(info.content);
			}
			else
			{
				std::cout << info.content << "\n";
			}
		}
		else if (strcmp(command, "/exit") == 0)
		{
			sendExit();
			break;
		}
		else
		{
			std::cout << "Command not found, check the available commands below:\n";
			printMenu();
		}

		// Clear buffer
		std::cin.ignore(10000, '\n');
		std::cin.clear();
	}
}

void client::runChat(char *chatPipeId)
{
	int fd;
	char chatPipe[64];
	sprintf(chatPipe, "/tmp/chat%s.pipe", chatPipeId);

	std::cout << "You have enter to chat: " << chatPipeId << "\n";
	// Clear buffer
	std::cin.ignore(10000, '\n');
	while (true)
	{
		std::cin.getline(info.content, sizeof(package::info::content));

		fd = OPEN(chatPipe, O_WRONLY, "An error has ocurred!\n");
		write(fd, &info, sizeof(package::info));
		close(fd);
	}
}

void client::runConc()
{
	if (fork() == 0)
	{
		int fd;
		package::info inf;

		while (true)
		{
			fd = OPEN(concPipe, O_RDONLY, "Error: could not open server pipe");
			read(fd, &inf, sizeof(package::info));
			close(fd);

			std::cout << inf.content << "\n";
		}
	}
}

void client::printMenu()
{
	std::cout << "\t/help \t\t\t Displays this dialog\n"
			  << "\t/list \t\t\t List all the available users\n"
			  << "\t/check \t\t\t Check chat conection requests\n"
			  << "\t/accept \t\t Accept an incomming chat\n"
			  << "\t/chat <id|name> \t Connect with a user to start a chat\n"
			  << "\t/available \t\t Make this user available to chat\n"
			  << "\t/exit \t\t\t Closes this session\n";
}

void client::sendInfo()
{
	if (fork() == 0)
	{
		int fd = OPEN(server_pipe, O_WRONLY, "Sorry the server is down :(\n");
		write(fd, &info, sizeof(package::info));
		close(fd);
		exit(0);
	}
}

void client::getInfo()
{
	int fd = OPEN(waitPipe, O_RDONLY, "Sorry the server is down :(\n");
	read(fd, &info, sizeof(package::info));
	close(fd);
}

void client::sendExit()
{
	if (connected)
	{
		info.action = package::exit;
		sprintf(info.content, "exit");
		sendInfo();
	}
}

void client::createPipe()
{
	char cmd[64] = "mkfifo ";
	strcat(cmd, waitPipe);
	strcat(cmd, " ");
	strcat(cmd, concPipe);
	strcat(cmd, " > /dev/null 2>&1");
	system(cmd);
}

void client::createPipe(char *pipe)
{
	char cmd[64] = "mkfifo ";
	strcat(cmd, pipe);
	strcat(cmd, " > /dev/null 2>&1");
	system(cmd);
}

void client::deletePipe()
{
	char cmd[64] = "rm ";
	strcat(cmd, waitPipe);
	strcat(cmd, " ");
	strcat(cmd, concPipe);
	strcat(cmd, " > /dev/null 2>&1");
	system(cmd);
}

///////////////////////////////////////////// [::  Entry Point  ::]

client *cli;
void closed(int i)
{
	cli->sendExit();
	delete cli;
	exit(0);
}

int main(int argc, char const *argv[])
{
	/* The following is to manage force closing */
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = closed;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	cli = new client();
	cli->run();
	delete cli;
	return 0;
}
