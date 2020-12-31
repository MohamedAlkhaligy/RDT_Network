#include "ClientHandler.h"

#define SERVER_FOLDER "server\\"

void ClientHandler::processClient() {

	int bytes = tcp->_recv(clientSocket, buffer, MAX_BUFFER);
	std::string filename = std::string(buffer, bytes);
	std::cout << filename << std::endl;

	FILE* fp = NULL;
	std::string myPath = SERVER_FOLDER + filename;
	if (!fopen_s(&fp, myPath.c_str(), "rb") && fp != NULL) {
		while (!feof(fp)) {
			int size = fread(buffer, 1, MAX_BUFFER, fp);
			int numBytes = tcp->_send(clientSocket, buffer, size);
			if (numBytes < 0) {
				break;
			}
		}
		fclose(fp);
	}

	// Send "\r\n.\r\n" so that the client stops receiveing.
	tcp->_send(clientSocket, SEPARATOR.c_str(), SEPARATOR.size());
}
