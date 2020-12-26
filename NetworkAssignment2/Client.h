#pragma once

#include "Utilities.h"
#include "TCP.h"

class Client
{
private:
	char buffer[MAX_BUFFER];
	SOCKET socketToServer;

	int init(TCP* tcp, const char* server, const char* serverPort);

public:
	int execute();
};
