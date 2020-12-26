#include "Utilities.h"

std::vector<std::string> Utilities::split(std::string s) {
	std::stringstream stringstream(s);
	std::istream_iterator<std::string> begin(stringstream);
	std::istream_iterator<std::string> end;
	std::vector<std::string> tokens(begin, end);
	return tokens;
}

std::string Utilities::getFileName(std::string path) {
	std::size_t found = path.rfind('\\');
	std::string fileName = path;
	if (found != std::string::npos) {
		int separator_index = found;
		fileName = std::string(&path[separator_index], &path[path.size()]);
	}
	return fileName;
}

int Utilities::sendRequest(SOCKET socket, std::string message) {
	int numBytes = send(socket, message.c_str(), message.size(), 0);
	int isSent = !SUCCESS;
	if (numBytes < 0) {
		std::cerr << "Client: Sending the request has failed" << std::endl;
	}
	else if (numBytes != message.size()) {
		std::cerr << "Client: Sending unexpected number of bytes" << std::endl;
	}
	else {
		isSent = SUCCESS;
	}
	return isSent;
}