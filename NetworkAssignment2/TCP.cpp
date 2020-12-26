#include "TCP.h"

int TCP::init() {
	// Initialize WinSock
	WSADATA data;
	WORD version = MAKEWORD(MAJOR, MINOR);
	int wsaResult = WSAStartup(version, &data);
	if (wsaResult != SUCCESS) {
		std::cerr << "Can't start WinSock, ERROR#" << wsaResult << std::endl;
		return wsaResult;
	}

	memset(&addrCriteria, 0, sizeof(addrCriteria));
	addrCriteria.ai_family = AF_UNSPEC;				// Any address family
	addrCriteria.ai_socktype = SOCK_DGRAM;			// Only datagram sockets
	addrCriteria.ai_flags = AI_PASSIVE;				// Accept on any address/port
	addrCriteria.ai_protocol = IPPROTO_UDP;			// Only UDP protocol

	return SUCCESS;
}

int TCP::_getaddrinfo(const char* IPAdress, const char* port) {
	return getaddrinfo(IPAdress, port, &addrCriteria, &servAddr);
}

SOCKET TCP::_socket() {
	return socket(servAddr->ai_family, servAddr->ai_socktype, servAddr->ai_protocol);
}

int TCP::_connect(SOCKET s) {
	// Three-way Handshake, third one will be sent with data.
	int success = 0;
	sendto(s, buffer, 0, 0, servAddr->ai_addr, servAddr->ai_addrlen);
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);

	struct timeval tv;
	tv.tv_sec = TIMEOUT_S;
	tv.tv_usec = TIMEOUT_M;

	while (!success) {
		if (select(s + 1, &readfds, NULL, NULL, &tv) == 0) {
			sendto(s, buffer, 0, 0, servAddr->ai_addr, servAddr->ai_addrlen);
			std::cout << "Handshake: timeout" << std::endl;
			tv.tv_sec = TIMEOUT_S;
			tv.tv_usec = TIMEOUT_M;
		} else {
			struct sockaddr_storage fromAddr;
			socklen_t fromAddrLen = sizeof(fromAddr);
			int numBytes = recvfrom(s, buffer, MAX_BUFFER, 0, (struct sockaddr*) &fromAddr, &fromAddrLen);
			if (numBytes < 0) {
				std::cout << "Client: no connection" << std::endl;
				return FAILURE;
			}

			// Assign new delegated server port.
			sockaddr_in sock;
			memcpy(&sock, buffer, numBytes);
			servAddr->ai_addr = (struct sockaddr*) &sock;
			servAddr->ai_addrlen = sizeof(sock);
			success = 1;
		}
	}
	return SUCCESS;
}

SOCKET TCP::_accept(SOCKET s, sockaddr* addr, int* addrlen) {

	// Receive a connection from a new client.
	struct sockaddr_storage fromAddr;
	socklen_t fromAddrLen = sizeof(fromAddr);
	int numBytes = recvfrom(s, buffer, MAX_BUFFER, 0, (struct sockaddr*)&fromAddr, &fromAddrLen);

	SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	sockaddr_in newSock;
	newSock.sin_addr.S_un.S_addr = INADDR_ANY;
	newSock.sin_port = htons(initialPort);
	newSock.sin_family = AF_INET;
	while (bind(client, (struct sockaddr*)&newSock, sizeof(newSock)) == SOCKET_ERROR) {
		newSock.sin_port = htons(initialPort++);
	}

	// 
	int success = 0;
	memcpy(buffer, &newSock, sizeof(newSock));
	sendto(s, buffer, sizeof(newSock), 0, (struct sockaddr*) &fromAddr, fromAddrLen);

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	struct timeval tv;
	tv.tv_sec = TIMEOUT_S;
	tv.tv_usec = TIMEOUT_M;

	while (!success) {
		if (select(s + 1, &readfds, NULL, NULL, &tv) == 0) {
			sendto(s, buffer, 0, 0, (struct sockaddr*)&fromAddr, fromAddrLen);
			std::cout << "Handshake: timeout" << std::endl;
			tv.tv_sec = TIMEOUT_S;
			tv.tv_usec = TIMEOUT_M;
		} else {
			struct sockaddr_storage fromAddr;
			socklen_t fromAddrLen = sizeof(fromAddr);
			int numBytes = recvfrom(s, buffer, MAX_BUFFER, 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
			if (numBytes < 0) {
				std::cout << "Client: no connection" << std::endl;
				return FAILURE;
			}
			memcpy(buffer, servAddr, numBytes);
			success = 1;
		}
	}

	

	return client;
}


int TCP::_send(SOCKET s, const char* data, int len) {
	int size = 0;
	
	if (len / MAX_DGRAM_PAYLOAD > packets.size()) {
		packets.resize(len / MAX_DGRAM_PAYLOAD + 1);
	}
	
	nextseqnum = 0;
	dupACKcount = 0;
	base = 0;
	lastPacketIndex = 0;

	lastPacketIndex = 0;
	//char c[MAX_DGRAM_PAYLOAD];
	//if (!isConnectionEstablishedS) {
	//	// Third-handshake message;
	//	struct packet pk = { 0, 0, nextseqnum++, *c};
	//	lastPacketIndex++;
	//	isConnectionEstablishedS = true;
	//}

	// Split data into packets
	// Bonus: Edit checksum later.
	int index = 0;
	while (len > 0) {
		struct packet pk;
		pk = { 0, min(MAX_DGRAM_PAYLOAD, len), nextseqnum };
		memcpy(pk.data, data + index, min(MAX_DGRAM_PAYLOAD, len));
		packets[lastPacketIndex] = pk;
		lastPacketIndex++;
		nextseqnum++;
		len -= MAX_DGRAM_PAYLOAD;
		index += MAX_DGRAM_PAYLOAD;
	}

	// Send the initial packets in the congestion window.
	for (int i = 0; i < cwnd && i < lastPacketIndex; i++) {
		char c[DATA_PACKET_SIZE];
		memcpy(c, &packets[i], DATA_PACKET_SIZE);
		sendto(s, c, DATA_PACKET_SIZE, 0, servAddr->ai_addr, servAddr->ai_addrlen);
		size += packets[i].len;
	}

	// FSM of TCP congestion control;
	int numBytes = recvfrom(s, buffer, MAX_BUFFER, 0, nullptr, nullptr);
	struct packet temp;
	memcpy(&temp, buffer, numBytes);

	while (base < lastPacketIndex) {
		fd_set readfds;
		FD_ZERO(&readfds);
		FD_SET(s, &readfds);
		struct timeval tv;
		tv.tv_sec = TIMEOUT_S;
		tv.tv_usec = TIMEOUT_M;

		int success = 0;
		while (!success) {
			if (select(s + 1, &readfds, NULL, NULL, &tv) == 0) {
				// Timeout
				sendto(s, buffer, 0, 0, servAddr->ai_addr, servAddr->ai_addrlen);
				std::cout << "Handshake: timeout" << std::endl;
				tv.tv_sec = TIMEOUT_S;
				tv.tv_usec = TIMEOUT_M;
			} else {
				// New Ack, or duplicate ack.
				int numBytes = recvfrom(s, buffer, MAX_BUFFER, 0, nullptr, nullptr);
				if (numBytes < 0) {
					std::cout << "Client: no connection" << std::endl;
					return FAILURE;
				}
				switch (currentState) {
					case SLOW_START: {

						break;
					}
					case CONGESTION_AVOIDANCE: {

						break;
					}
					case FAST_RECOVERY: {

						break;
					}
				}
			}
		}
	}
	return size;
}

int TCP::_bind(SOCKET s) {
	return bind(s, servAddr->ai_addr, servAddr->ai_addrlen);
}

int TCP::_listen() {
	return 0;
}

int TCP::_recv(SOCKET s, const char* buff, int len) {
	struct sockaddr_storage fromAddr;
	socklen_t fromAddrLen = sizeof(fromAddr);
	int numBytes = 0;
	struct packet pk = {0, 0, -1};
	while (true) {
		int numBytes = recvfrom(s, buffer, MAX_BUFFER, 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
		memcpy(&pk, buffer, DATA_PACKET_SIZE);
		if (pk.seqno == expectedseqnum) {
			// Send ack.
			memcpy(&buff, pk.data, pk.len);
			pk = { 0, 0, expectedseqnum };
			memcpy(buffer, &pk, ACK_PACKET_SIZE);
			sendto(s, buffer, MAX_BUFFER, 0, servAddr->ai_addr, servAddr->ai_addrlen);
			expectedseqnum++;
			// Return recevied data.
		} else {
			// Send duplicate ack.
			memcpy(buffer, &pk, ACK_PACKET_SIZE);
			sendto(s, buffer, MAX_BUFFER, 0, servAddr->ai_addr, servAddr->ai_addrlen);
		}
		return pk.len;
	}
}

void TCP::_closesocket(SOCKET s) {
	closesocket(s);
}