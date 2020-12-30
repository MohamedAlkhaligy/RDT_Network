#pragma once

#include <vector>

#include "Utilities.h"

static const double TIMEOUT_S = 90.0;
static const double TIMEOUT_M = 0;
static const int TRIPLET = 3;

class TCP
{
private:

	enum states {SLOW_START, CONGESTION_AVOIDANCE, FAST_RECOVERY};

	struct addrinfo addrCriteria;
	struct addrinfo* servAddr;
	SOCKET destinationSocket;
	int initialPort = 1024;

	states currentState = SLOW_START;

	uint32_t ssthread = INITIAL_THRESHOLD / DATA_PACKET_SIZE;
	uint32_t cwnd = 1;
	uint32_t nextseqnum = 1;
	int dupACKcount = 0;
	int base = 1;
	int lastPacketIndex = 1;
	bool isConnectionEstablishedS = false;
	bool isConnectionEstablishedR = false;
	struct ack_packet ack = { 0, 0, 0 };
	char buffer[MAX_BUFFER];

	uint32_t expectedseqnum = 1;

	void rdt_send();

	std::vector<struct packet> packets;

public:

	// The source IPAdress, and port will be intitialized by the OS.
	TCP() {
		packets = std::vector<struct packet>(PACKETS);
	}

	int init();

	SOCKET _accept(SOCKET s, sockaddr* addr, int* addrlen);

	int _getaddrinfo(const char* IPAdress, const char* port);

	SOCKET _socket();

	int _bind(SOCKET s);

	int _listen();

	int _connect(SOCKET s);

	int _send(SOCKET s, const char* data, int len);

	int _recv(SOCKET s, char* buffer, int len);

	// Close TCP connection.
	void _closesocket(SOCKET s);
};

