#include "TCP.h"

int TCP::_connect(SOCKET s, const sockaddr* servAddr, int servAddrLen) {
	// Connect to server.
	addr = servAddr;
	addrlen = servAddrLen;
	if (connect(s, addr, addrlen) < 0) {
		std::cout << "Error : Connect Failed" << std::endl;
		return FAILURE;
	}

	// Three-way Handshake, third one will be sent with data.
	// Downgraded to two-way handshake.
	sendto(s, buffer, 0, 0);
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);

	struct timeval tv;
	tv.tv_sec = TIMEOUT_S;
	tv.tv_usec = TIMEOUT_M;

	int success = 0;
	while (!success) {
		if (select(s + 1, &readfds, NULL, NULL, &tv) == 0) {
			sendto(s, buffer, 0, 0, addr, addrlen);
			std::cout << "Handshake: timeout" << std::endl;
			tv.tv_sec = TIMEOUT_S;
			tv.tv_usec = TIMEOUT_M;
		} else {
			struct sockaddr_storage fromAddr;
			socklen_t fromAddrLen = sizeof(fromAddr);
			int numBytes = recvfrom(s, buffer, sizeof(sockaddr_in), 0, (struct sockaddr*) &fromAddr, &fromAddrLen);
			if (numBytes < 0) {
				std::cout << "Client: no connection" << std::endl;
				return FAILURE;
			}

			// Assign new delegated server port.
			std::cout << "Connection Established" << std::endl;
			sockaddr_in sock;
			memcpy(&sock, buffer, numBytes);
			// addr = (struct sockaddr*) &sock;
			// addrlen = sizeof(sock);
			success = 1;
		}
	}
	return SUCCESS;
}

SOCKET TCP::_accept(SOCKET s) {
	// Receive a connection from a new client.
	struct sockaddr_storage fromAddr;
	socklen_t fromAddrLen = sizeof(fromAddr);
	int numBytes = recvfrom(s, buffer, 0, 0, (struct sockaddr*) &fromAddr, &fromAddrLen);

	// Save client address.
	addr = (struct sockaddr*) &fromAddr;
	addrlen = fromAddrLen;

	// Delegate the new client to a new server socket.
	SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	sockaddr_in newSock;
	newSock.sin_addr.S_un.S_addr = INADDR_ANY;
	newSock.sin_port = htons(initialPort);
	newSock.sin_family = AF_INET;
	/*while (bind(client, (struct sockaddr*)&newSock, sizeof(newSock)) == SOCKET_ERROR) {
		newSock.sin_port = htons(initialPort++);
	}*/

	memcpy(buffer, &newSock, sizeof(sockaddr_in));
	sendto(s, buffer, sizeof(sockaddr_in), 0, addr, addrlen);
	return client;
}


int TCP::_send(SOCKET s, const char* data, int len) {
	
	if (len / MAX_DGRAM_PAYLOAD > packets.size()) {
		packets.resize(len / MAX_DGRAM_PAYLOAD + MARGIN);
	}

	nextseqnum = 1;
	dupACKcount = 0;
	base = 1;
	lastPacketIndex = 1;
	int lastIndexSent = 1;
	int size = 0;

	// Split data into packets
	// Bonus: Edit checksum later.
	int index = 0;
	while (len > 0) {
		struct packet pk;
		pk = { 0, (uint16_t) min(MAX_DGRAM_PAYLOAD, len), nextseqnum, false};
		memcpy(pk.data, data + index, min(MAX_DGRAM_PAYLOAD, len));
		packets[lastPacketIndex] = pk;
		lastPacketIndex++;
		nextseqnum++;
		len -= MAX_DGRAM_PAYLOAD;
		index += MAX_DGRAM_PAYLOAD;
	}

	// Final packet to be sent.
	packets[lastPacketIndex] = { 0, 0, nextseqnum++, true};

	// Send the initial packets in the congestion window.
	for (int i = base; i <= cwnd && i < lastPacketIndex; i++, lastIndexSent++) {
		char t[MAX_BUFFER];
		memcpy(t, &packets[i], DATA_PACKET_SIZE);
		sendto(s, t, DATA_PACKET_SIZE, 0, addr, addrlen);
		std::cout << "Sending: Packet " << packets[i].seqno << std::endl;
		size += packets[i].len;
	}

	// FSM of TCP congestion control;

	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	struct timeval tv;
	tv.tv_sec = TIMEOUT_S;
	tv.tv_usec = TIMEOUT_M;

	int counter = 0;
	while (base < lastPacketIndex) {
		// Timeout
		if (select(s + 1, &readfds, NULL, NULL, &tv) == 0) {
			ssthread = cwnd / 2;
			cwnd = 1;
			dupACKcount = 0;
			// Retransmit missing segments (Go-Back-N)
			std::cout << "Timeout have occured:" << std::endl;
			for (int i = base; i <= base + cwnd && i < lastPacketIndex; i++) {
				char c[DATA_PACKET_SIZE];
				memcpy(c, &packets[i], DATA_PACKET_SIZE);
				sendto(s, c, DATA_PACKET_SIZE, 0, addr, addrlen);
				std::cout << "Resending: Packet " << packets[i].seqno << std::endl;
				size += packets[i].len;
			}
			switch (currentState) {
				case SLOW_START: break;
				case CONGESTION_AVOIDANCE: currentState = SLOW_START; break;
				case FAST_RECOVERY:	currentState = SLOW_START; break;
			}
			tv.tv_sec = TIMEOUT_S;
			tv.tv_usec = TIMEOUT_M;
		} else {
			struct sockaddr_storage fromAddr;
			socklen_t fromAddrLen = sizeof(fromAddr);
			int numBytes = recvfrom(s, buffer, DATA_PACKET_SIZE, 0, (struct sockaddr*)&fromAddr, &fromAddrLen);
			if (numBytes < 0) {
				std::cout << "Client: no connection" << std::endl;
				return FAILURE;
			}
			// check whether duplicate or ack.
			struct ack_packet temp;
			memcpy(&temp, buffer, ACK_PACKET_SIZE);

			// new ACK
			if (temp.ackno == base) {
				dupACKcount = 0;
				base++;
				switch (currentState) {
				case SLOW_START: {
					cwnd++;
					for (int i = lastIndexSent; i <= base + cwnd && i < lastPacketIndex; i++, lastIndexSent++) {
						char c[DATA_PACKET_SIZE];
						memcpy(c, &packets[i], DATA_PACKET_SIZE);
						sendto(s, c, DATA_PACKET_SIZE, 0, addr, addrlen);
						std::cout << "Sending: Packet " << packets[i].seqno << std::endl;
						size += packets[i].len;
					}
					if (cwnd >= ssthread) currentState = CONGESTION_AVOIDANCE;
					break;
				}
				case CONGESTION_AVOIDANCE: {
					counter++;
					if (counter == cwnd) {
						cwnd++;
						counter = 0;
					} else {
						counter++;
					}
					for (int i = lastIndexSent; i <= base + cwnd && i < lastPacketIndex; i++, lastIndexSent++) {
						char c[DATA_PACKET_SIZE];
						memcpy(c, &packets[i], DATA_PACKET_SIZE);
						sendto(s, c, DATA_PACKET_SIZE, 0, addr, addrlen);
						std::cout << "Sending: Packet " << packets[i].seqno << std::endl;
						size += packets[i].len;
					}
					break;
				} 
				case FAST_RECOVERY: {
					cwnd = ssthread;
					currentState = CONGESTION_AVOIDANCE;
					break;
				}
				}
			} 
			// Duplicate ack
			else if (temp.ackno < base) {
				switch (currentState) {
				case SLOW_START: {
					dupACKcount++;
					if (dupACKcount == TRIPLET) {
						ssthread = cwnd / 2;
						cwnd = ssthread + 3;
						for (int i = base; i <= base + cwnd && i < lastPacketIndex; i++) {
							char c[DATA_PACKET_SIZE];
							memcpy(c, &packets[i], DATA_PACKET_SIZE);
							sendto(s, c, DATA_PACKET_SIZE, 0, addr, addrlen);
							std::cout << "Resending: Packet " << packets[i].seqno << std::endl;
							size += packets[i].len;
							currentState = FAST_RECOVERY;
							dupACKcount = 0;
						}
					}
					break;
				}
				case CONGESTION_AVOIDANCE: {
					dupACKcount++;
					if (dupACKcount == TRIPLET) {
						ssthread = cwnd / 2;
						cwnd = ssthread + 3;
						for (int i = base; i <= base + cwnd && i < lastPacketIndex; i++) {
							char c[DATA_PACKET_SIZE];
							memcpy(c, &packets[i], DATA_PACKET_SIZE);
							sendto(s, c, DATA_PACKET_SIZE, 0, addr, addrlen);
							std::cout << "Resending: Packet " << packets[i].seqno << std::endl;
							size += packets[i].len;
							currentState = FAST_RECOVERY;
							dupACKcount = 0;
						}
					}
					break;
				} 
				case FAST_RECOVERY: {
					cwnd += 1;
					for (int i = base; i <= base + cwnd && i < lastPacketIndex; i++, lastIndexSent++) {
						char c[DATA_PACKET_SIZE];
						memcpy(c, &packets[i], DATA_PACKET_SIZE);
						sendto(s, c, DATA_PACKET_SIZE, 0, addr, addrlen);
						std::cout << "Resending: Packet " << packets[i].seqno << std::endl;
						size += packets[i].len;
					}
					break;
				}
				}
			}

			
		}
	}
	return size;
}

int TCP::_listen() {
	return 0;
}

int TCP::_recv(SOCKET s, char* buff, int len) {
	struct sockaddr_storage fromAddr;
	socklen_t fromAddrLen = sizeof(fromAddr);
	int numBytes = 0;
	struct packet pk = {0, 0, 0, false};
	while (true) {
		numBytes = recvfrom(s, buffer, DATA_PACKET_SIZE, 0, (struct sockaddr*) &fromAddr, &fromAddrLen);
		std::cout << "Receivng: Packet " << pk.seqno << std::endl;
		memcpy(&pk, buffer, numBytes);
		if (pk.seqno == expectedseqnum) {
			// Deliver data, and Send ack.
			memcpy(buff, &pk.data, pk.len);
			ack = { 0, 0, expectedseqnum, s };
			memcpy(buffer, &ack, ACK_PACKET_SIZE);
			sendto(s, buffer, ACK_PACKET_SIZE, 0, addr, addrlen);
			expectedseqnum++;
			// Return recevied data.
			break;
		} else {
			// Send duplicate ack.
			memcpy(buffer, &ack, ACK_PACKET_SIZE);
			sendto(s, buffer, MAX_BUFFER, 0, addr, addrlen);
		}
	}
	return (int)pk.len;
}

void TCP::_closesocket(SOCKET s) {
	closesocket(s);
}