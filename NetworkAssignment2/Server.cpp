#include "server.h"

#define RUNNING true

int Server::init(TCP* tcp, const char* server, const char* port) {

	// Initialize WinSock
	WSADATA data;
	WORD version = MAKEWORD(MAJOR, MINOR);
	int wsaResult = WSAStartup(version, &data);
	if (wsaResult != SUCCESS) {
		std::cerr << "Can't start WinSock, ERROR#" << wsaResult << std::endl;
		return wsaResult;
	}

	struct addrinfo* servAddr;
	struct addrinfo addrCriteria;
	memset(&addrCriteria, 0, sizeof(addrCriteria));
	addrCriteria.ai_family = AF_INET;				// Any address family
	addrCriteria.ai_socktype = SOCK_DGRAM;			// Only datagram sockets
	addrCriteria.ai_flags = AI_PASSIVE;				// Accept on any address/port
	addrCriteria.ai_protocol = IPPROTO_UDP;			// Only UDP protocol

	int status;
	if ((status = getaddrinfo(server, port, &addrCriteria, &servAddr)) != 0) {
		std::cerr << "getaddrinfo error: %s\n" << gai_strerror(status);
		return status;
	}

	// Create a socket
	mySocket = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
	std::cout << "Server Socket: " << mySocket << std::endl;
	if (mySocket == INVALID_SOCKET) {
		std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
		WSACleanup();
		return SOCKET_ERROR;
	}

	// Bind the ip address and port to a socket
	if (bind(mySocket, servAddr->ai_addr, servAddr->ai_addrlen) == SOCKET_ERROR) {
		std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
		return WSAGetLastError();
	}
	return 0;
}

int Server::run() {

	// Try to read the client.in file and execute it.
	// Note: No error checking is done.
	try {
		std::ifstream clientFile(SERVER_DEFAULT_PATH);
		std::string port, seed, lossProbability;
		getline(clientFile, port);
		getline(clientFile, seed);
		getline(clientFile, lossProbability);

		// Check if all arguments needed exist.
		if (port.empty() || seed.empty() || lossProbability.empty()) {
			std::cout << "server.in: information(s) is(are) missing." << std::endl;
			return FAILURE;
		}

		TCP* tcp = new TCP();

		// Initialize server's socket.
		if (init(tcp, SERVER.c_str(), port.c_str()) != SUCCESS) return FAILURE;

		// Run the server.
		while (RUNNING) {
			sockaddr addr;
			int addrlen;
			SOCKET clientSocket = tcp->_accept(mySocket, &addr, &addrlen);

			// Delegate the new client to another server port.
			ClientHandler* client = new ClientHandler(clientSocket, stoi(seed), 
				stod(lossProbability), addr, addrlen);
		}

	} catch (const std::ifstream::failure& e) {
		std::cout << e.what() << std::endl;
	}

	return SUCCESS;
}