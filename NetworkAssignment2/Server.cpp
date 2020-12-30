#include "server.h"

#define RUNNING true
#define SERVER_FOLDER "server\\"

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
	std::cout << "Socket: " << mySocket << std::endl;
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

		// Initialize client's socket conneciton.
		if (init(tcp, SERVER.c_str(), port.c_str()) != SUCCESS) return FAILURE;



		while (RUNNING) {
			SOCKET clientSocket = tcp->_accept(mySocket);
			// int bytes = tcp->_recv(clientSocket, buffer, MAX_BUFFER);
			// ***** Change socket
			int bytes = tcp->_recv(mySocket, buffer, MAX_BUFFER);
			std::string filename = std::string(buffer, bytes);
			std::cout << filename << std::endl;

			FILE* fp = NULL;
			std::string myPath = SERVER_FOLDER + filename;
			if (!fopen_s(&fp, myPath.c_str(), "rb") && fp != NULL) {
				while (!feof(fp)) {
					int size = fread(buffer, 1, MAX_BUFFER, fp);
					// **** Change socket
					int numBytes = tcp->_send(mySocket, buffer, size);
					if (numBytes < 0) {
						break;
					}
				}
				fclose(fp);
				break;
			}

			//ClientsHandler* client = new ClientsHandler(clientSocket);
			// clients.push_back(client);
		}
		// Send "\r\n\r\n" so that the client stops receiveing.
		tcp->_send(mySocket, SEPARATOR.c_str(), SEPARATOR.size());

	} catch (const std::ifstream::failure& e) {
		std::cout << e.what() << std::endl;
	}
}