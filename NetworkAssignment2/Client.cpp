#include "Client.h"

#define CLIENT_FOLDER "client\\"

int Client::init(TCP* tcp, const char* server, const char* serverPort) {

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
	if ((status = getaddrinfo(server, serverPort, &addrCriteria, &servAddr)) != 0) {
		std::cerr << "getaddrinfo error: %s\n" << gai_strerror(status);
		return status;
	}

	// Create a socket
	socketToServer = socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
	if (socketToServer == INVALID_SOCKET) {
		std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
		WSACleanup();
		return SOCKET_ERROR;
	}

	// try to find a port, and bind the IP address and port to a socket
	sockaddr_in clieAddr;
	clieAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	clieAddr.sin_port = htons(initialPort);
	clieAddr.sin_family = AF_INET;
	while (bind(socketToServer, (struct sockaddr*) &clieAddr, sizeof(clieAddr)) == SOCKET_ERROR) {
		clieAddr.sin_port = htons(initialPort++);
	}

	// Connect to the server
	int connection = tcp->_connect(socketToServer, servAddr->ai_addr, servAddr->ai_addrlen);
	if (connection == FAILURE) {
		std::cerr << "Can't connect to server, ERROR#" << WSAGetLastError() << std::endl;
		tcp->_closesocket(socketToServer);
		WSACleanup();
		return SOCKET_ERROR;
	}

	return 0;
}

int Client::execute() {

	// Try to read the client.in file and execute it.
	// Note: No format error checking is done.
	try {
		std::ifstream clientFile(CLIENT_DEFAULT_PATH);
		std::string server, serverPort, filename;
		getline(clientFile, server);
		getline(clientFile, serverPort);
		getline(clientFile, filename);

		// Check if all arguments needed exist.
		if (server.empty() || serverPort.empty() || filename.empty()) {
			std::cout << "client.in: information(s) is(are) missing." << std::endl;
			return FAILURE;
		}

		TCP* tcp = new TCP();

		// Initialize client's socket conneciton.
		if (init(tcp, server.c_str(), serverPort.c_str()) != SUCCESS) return FAILURE;

		// Send the requested filename to the server.
		tcp->_send(socketToServer, filename.c_str(), filename.size());

		// Receive the requested file from the server.
		std::string clientpath = CLIENT_FOLDER + filename;
		FILE* file = NULL;
		if (!fopen_s(&file, clientpath.c_str(), "wb") && file != NULL) {
			int currentFileSize = 0, numBytesRcvd = 1;
			while (numBytesRcvd > 0) {
				numBytesRcvd = tcp->_recv(socketToServer, buffer, MAX_BUFFER);

				if (numBytesRcvd < 0) {
					std::cerr << "Client: Receiving file has failed" << std::endl;
					return FAILURE;
				} else if (numBytesRcvd == 0) {
					std::cerr << "Client: Connection Closed" << std::endl;
					return FAILURE;
				} else {
					// If separator was sent by server, then break. Otherwise contine
					// appending to the existing file.
					std::ostringstream oss;
					oss << std::string(buffer, numBytesRcvd);
					std::size_t found = oss.str().rfind(SEPARATOR);
					if (found != std::string::npos) break;
					fwrite(buffer, numBytesRcvd, 1, file);
				}
			}
			fclose(file);
		}

		// Close the connection between the client and server.
		tcp->_closesocket(socketToServer);

	} catch (const std::ifstream::failure& e) {
		std::cout << e.what() << std::endl;
	}

	return SUCCESS;
}

