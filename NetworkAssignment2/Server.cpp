#include "server.h"

#define RUNNING true

int Server::init(TCP* tcp, const char* server, const char* port) {

	// Initialize WinSock
	if (tcp->init() != SUCCESS) return FAILURE;

	// Fill in a hint structure
	int status;
	if (status = tcp->_getaddrinfo(server, port) != SUCCESS) {
		std::cerr << "getaddrinfo error: %s\n" << gai_strerror(status);
		return status;
	}

	// Create a sokcet
	mySocket = tcp->_socket();
	if (mySocket == INVALID_SOCKET) {
		std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
	}

	// Bind the ip address and port to a socket
	if (tcp->_bind(mySocket) == SOCKET_ERROR) {
		std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
		return WSAGetLastError();
	}

	//// Tell WinSock the socket is for listening
	//if (listen(mySocket, SOMAXCONN) == SOCKET_ERROR) {
	//	std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
	//	return WSAGetLastError();
	//}

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

		// Initialize client's socket conneciton.
		if (init(tcp, SERVER.c_str(), port.c_str()) != SUCCESS) return FAILURE;

		while (RUNNING) {
			SOCKET clientSocket = tcp->_accept(mySocket, nullptr, nullptr);
			char buffer[MAX_BUFFER];
			int bytes = tcp->_recv(clientSocket, buffer, MAX_BUFFER);
			std::cout << std::string(buffer, bytes) << std::endl;
			//ClientsHandler* client = new ClientsHandler(clientSocket);
			// clients.push_back(client);
		}

	} catch (const std::ifstream::failure& e) {
		std::cout << e.what() << std::endl;
	}
}