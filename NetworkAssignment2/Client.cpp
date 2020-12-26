#include "Client.h"

#define CLIENT "client\\"

int Client::init(TCP* tcp, const char* server, const char* serverPort) {

	// Initialize WinSock
	if (tcp->init() != SUCCESS) return FAILURE;

	int status;
	if ((status = tcp->_getaddrinfo(server, serverPort)) != 0) {
		std::cerr << "getaddrinfo error: %s\n" << gai_strerror(status);
		return status;
	}

	// Create a sokcet
	socketToServer = tcp->_socket();
	if (socketToServer == INVALID_SOCKET) {
		std::cerr << "Can't create socket, ERROR#" << WSAGetLastError() << std::endl;
		WSACleanup();
		return SOCKET_ERROR;
	}

	// Connect to the server
	int connection = tcp->_connect(socketToServer);
	if (connection == SOCKET_ERROR) {
		std::cerr << "Can't connect to server, ERROR#" << WSAGetLastError() << std::endl;
		tcp->_closesocket(socketToServer);
		WSACleanup();
		return SOCKET_ERROR;
	}

	return 0;
}

int Client::execute() {

	// Try to read the client.in file and execute it.
	// Note: No error checking is done.
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
		std::string clientpath = CLIENT + filename;
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

