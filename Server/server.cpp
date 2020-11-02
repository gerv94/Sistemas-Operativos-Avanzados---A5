#include "server.hpp"

server::server(/* args */)
{
	pid = getpid();
}

server::~server()
{
	deletePipe();
	LOG("Server closed");
}

void server::run()
{
	LOG("Running server");
	createPipe();

	LOG("Waiting for a user:");
	getInfo();

	while (!clients.empty())
	{
		getInfo();
	}
}

void server::runChat(int id0, int id1)
{
	if (fork() == 0)
	{
		int fd, index;
		char content[256];
		package::info inf;
		char chatPipe[64];
		sprintf(chatPipe, "/tmp/chat%d.pipe", getpid());
		createChatPipe(chatPipe);

		inf.id = id0;
		sprintf(inf.content, "%d", getpid());
		inf.ok = true;
		sendInfo(inf);

		inf.id = id1;
		sendInfo(inf);

		//Start listening for messages
		LOG("Waiting for clients input");

		while (true)
		{
			fd = OPEN(chatPipe, O_RDONLY, "Error: could not open chat pipe");
			read(fd, &inf, sizeof(package::info));
			close(fd);

			LOG("User (" << inf.id << ") is sending " << inf.content);

			index = getClientIndex(inf.id);

			if (inf.id == id0)
			{
				inf.id = id1;
			}
			else
			{
				inf.id = id0;
			}

			sprintf(content, "[%s] : %s ", clients[index].name, inf.content);
			sprintf(inf.content, content);
			sendMessage(inf);
		}
	}
}

void server::getInfo()
{
	int fd;
	package::info info;

	fd = OPEN(pipe, O_RDONLY, "Error: could not open server pipe");

	read(fd, &info, sizeof(package::info));
	close(fd);

	switch (info.action)
	{
	case package::login:
	{
		LOG("User (" << info.id << ") is asking to login as " << info.content << "\n");
		//TODO: forbide access if user name already exists
		if (clients.size() < max_users)
		{
			LOG("Allowing user [" << info.id << "]");
			client client;
			client.id = info.id;
			sprintf(client.name, info.content);
			client.status = e_status::accept;
			clients.push_back(client);

			printUsers();

			sprintf(info.content, "You have been sucessfully logged!!");
			info.ok = true;
			sendInfo(info);
		}
		else
		{
			sprintf(info.content, "The stack is full try again later");
			info.ok = false;
			sendInfo(info);
			LOG("Forbiden user [" << info.id << "] stack full");
		}
		break;
	}
	case package::list:
	{
		LOG("User (" << info.id << ") is asking for the user list");
		sprintf(info.content, "Current active users:\n");
		strcat(info.content, "    [ Id ] Name :\tStatus\n");
		strcat(info.content, "---------------------------------\n");
		for (client client : clients)
		{
			strcat(info.content, "    [ ");
			sprintf(info.content, "%s%d", info.content, client.id);
			strcat(info.content, " ] ");
			strcat(info.content, client.name);
			strcat(info.content, " :\t");
			switch (client.status)
			{
			case e_status::accept:
				strcat(info.content, "Available");
				break;
			case e_status::messaging:
				strcat(info.content, "In a chat");
				break;
			case e_status::reject:
				strcat(info.content, "Not available");
				break;
			case e_status::pending:
				strcat(info.content, "Bussy");
				break;
			}
			if (client.id == info.id)
			{
				strcat(info.content, "\t(You)");
			}
			strcat(info.content, "\n");
		}
		info.ok = true;
		sendInfo(info);
		break;
	}
	case package::connect:
	{
		//TODO: allow user to connect by id and avoid chatting with himself
		int client_index = -1;
		LOG("User (" << info.id << ") is asking to chat with: " << info.content);

		//Search for the user
		for (int index = 0; index < clients.size(); index++)
		{
			if (strcmp(info.content, clients[index].name) == 0)
			{
				if (strcmp(clients[index].name, info.content) == 0)
				{
					LOG("User (" << info.id << ") is asking to chat with himself");
				}
				else
				{
					client_index = index;
				}
				break;
			}
		}

		if (client_index != -1)
		{
			if (clients[client_index].status == e_status::accept)
			{
				LOG("The user has been found and is available to chat with");
				//TODO: send message to user of an incomming chat
				int asking_id = info.id;
				info.id = clients[client_index].id;

				clients[client_index].status = pending;
				clients[client_index].partner = asking_id;
				sprintf(info.content, "[SERVER] : A user is trying to connect with you, type /check to find out!");
				sendMessage(info);
			}
			else
			{
				LOG("The user has been found but is not available to chat");
				sprintf(info.content, "The user has been found but is not available to chat");
				info.ok = false;
				sendInfo(info);
			}
		}
		else
		{
			LOG("The user has not been found");
			sprintf(info.content, "The user has not been found");
			info.ok = false;
			sendInfo(info);
		}
		break;
	}
	case package::check:
	{
		LOG("User (" << info.id << ") is checking for incomming connections");
		int currentIndex = getClientIndex(info.id);
		int partnerIndex = getClientIndex(clients[currentIndex].partner);

		sprintf(info.content, clients[partnerIndex].name);
		strcat(info.content, " is trying to contact with you, type /accept to start this chat");

		sendInfo(info);
		break;
	}
	case package::accept:
	{
		LOG("User (" << info.id << ") is trying to accept conection");

		int currentIndex = getClientIndex(info.id);
		// Check if the user indeed have a connection
		if (clients[currentIndex].status == pending)
		{
			int partnerIndex = getClientIndex(clients[currentIndex].partner);

			clients[currentIndex].status = messaging;
			clients[partnerIndex].status = messaging;

			runChat(info.id, clients[partnerIndex].id);
		}
		else
		{
			sprintf(info.content, "You have no incomming chat connections");
			info.ok = false;
			sendInfo(info);
		}

		break;
	}
	case package::exit:
	{
		LOG("User (" << info.id << ") is leaving\n");
		for (int index = 0; index < clients.size(); index++)
		{
			if (clients[index].id == info.id)
			{
				clients.erase(clients.begin() + index);
				LOG("Removing user from table");
				break;
			}
		}
		printUsers();
		break;
	}
	default:
		break;
	}
}

void server::printUsers()
{
	LOG("Currently Active Users: " << clients.size());
}

void server::sendInfo(package::info info)
{
	if (fork() == 0)
	{
		char client_pipe[64];
		sprintf(client_pipe, "/tmp/wait%d.pipe", info.id);

		int fd = OPEN(client_pipe, O_WRONLY, "Error! Could not connect with user [" << info.id << "] " << client_pipe);

		write(fd, &info, sizeof(package::info));
		close(fd);

		exit(0);
	}
}

void server::sendMessage(package::info info)
{
	if (fork() == 0)
	{
		char client_pipe[64];
		sprintf(client_pipe, "/tmp/conc%d.pipe", info.id);

		int fd = OPEN(client_pipe, O_WRONLY, "Error! Could not connect with user [" << info.id << "] " << client_pipe);

		write(fd, &info, sizeof(package::info));
		close(fd);

		exit(0);
	}
}

int server::getClientIndex(int pid)
{
	for (int index = 0; index < clients.size(); index++)
	{
		if (clients[index].id == pid)
		{
			return index;
		}
	}
	return -1;
}

void server::createPipe()
{
	char cmd[64] = "mkfifo ";
	strcat(cmd, pipe);
	strcat(cmd, " > /dev/null 2>&1");
	system(cmd);
}

void server::createChatPipe(char *chatPipe)
{
	char cmd[64] = "mkfifo ";
	strcat(cmd, chatPipe);
	strcat(cmd, " > /dev/null 2>&1");
	system(cmd);
}

void server::deletePipe()
{
	char cmd[64] = "rm ";
	strcat(cmd, pipe);
	strcat(cmd, " /tmp/chat*.pipe");
	strcat(cmd, " > /dev/null 2>&1");
	system(cmd);
}

///////////////////////////////////////////// [::  Entry Point  ::]

server *serv;
void closed(int i)
{
	delete serv;
}

int main(int argc, char const *argv[])
{
	/* The following is to manage force closing */
	struct sigaction sigIntHandler;
	sigIntHandler.sa_handler = closed;
	sigemptyset(&sigIntHandler.sa_mask);
	sigIntHandler.sa_flags = 0;
	sigaction(SIGINT, &sigIntHandler, NULL);

	serv = new server();
	serv->run();
	delete serv;
	return 0;
}
