#pragma once

#include "Utilities.h"
#include "TCP.h"

class Client
{
private:
	char buffer[MAX_BUFFER];
	int initialPort = 2048;
	struct addrinfo* servAddr;
	SOCKET socketToServer;

	int init(TCP* tcp, const char* server, const char* serverPort);

public:
	int execute();
};
