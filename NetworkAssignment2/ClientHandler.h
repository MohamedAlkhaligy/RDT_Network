#pragma once

#include "Utilities.h"
#include "TCP.h"

class ClientHandler {

private:
	SOCKET clientSocket;
	std::thread* client;
	TCP* tcp;
	int seed;
	double lossProbability;
	double corruptionProbability;
	char buffer[MAX_BUFFER];

	void processClient();

public:

	ClientHandler(SOCKET _clientSocket, int _seed, double _lossProbability, double _corruptionProbability,
		sockaddr _addr, int _addrlen) {

		sendto(_clientSocket, NULL, 0, 0, &_addr, _addrlen);

		seed = _seed;
		lossProbability = _lossProbability;
		clientSocket = _clientSocket;
		corruptionProbability = _corruptionProbability;
		tcp = new TCP(_addr, _addrlen);
		client = new std::thread([this]() { processClient(); });
		std::cout << "Delegated - Socket: " << clientSocket << std::endl;
	}

};

