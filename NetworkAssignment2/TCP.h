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
	std::string sstates[TRIPLET] = {"Slow Start", "Congestion Avoidance", "Fast Recovery"};

	struct addrinfo addrCriteria;
	const sockaddr* addr;
	int addrlen;
	SOCKET destinationSocket;
	int initialPort = 1024;

	states currentState = SLOW_START;

	// uint32_t ssthread = INITIAL_THRESHOLD / DATA_PACKET_SIZE;
	uint32_t ssthread = 32;
	uint32_t cwnd = 1;
	int counter = 0;
	uint32_t nextseqnum = 1;
	int dupACKcount = 0;
	int base = 1;
	int lastPacketIndex = 1;
	bool isConnectionEstablishedS = false;
	bool isConnectionEstablishedR = false;
	struct ack_packet ack = { 0, 0, 0 };
	char buffer[MAX_BUFFER];

	uint32_t expectedseqnum = 1;

	std::vector<struct packet> packets;

public:

	// The source IPAdress, and port will be intitialized by the OS.
	TCP() {
		packets = std::vector<struct packet>(PACKETS);
	}

	TCP(sockaddr _addr, int _addrlen) {
		packets = std::vector<struct packet>(PACKETS);
		addr = new sockaddr(_addr);
		addrlen = int(_addrlen);
	}

	SOCKET _accept(SOCKET s, sockaddr* _addr, int* _addrlen);

	void setAddr(sockaddr _addr, int _addrlen);

	int _connect(SOCKET s, const sockaddr* _addr, int _addrlen);

	int _send(SOCKET s, const char* data, int len);

	int _recv(SOCKET s, char* buffer, int len);

	// Close TCP connection.
	void _closesocket(SOCKET s);
};

