#pragma once

// #include "ClientsHandler.h"
#include "Utilities.h"
#include "ClientHandler.h"
#include "TCP.h"
#include <list>
#include <thread>

#pragma comment (lib, "ws2_32.lib")

// The delay when one socket is connected

class Server
{
private:
	SOCKET mySocket;
	double timeout; // in seconds

public:

	int init(TCP* tcp, const char* server, const char* serverPort);

	int run();

};