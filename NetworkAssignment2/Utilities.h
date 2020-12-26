#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <iterator>
#include <direct.h>
#include <WS2tcpip.h>
#include <time.h>
#include <chrono>
#include <thread>

static const int MAX_BUFFER = 4096;
static const int MAX_DGRAM_PAYLOAD = 512;
static const int INITIAL_THRESHOLD = 65536;
static const int DATA_PACKET_SIZE = 528;
static const int ACK_PACKET_SIZE = 12;
static const int MAJOR = 2;
static const int MINOR = 2;
static const int SUCCESS = 0;
static const int FAILURE = 1;
static const int PACKETS = 8192;
static const std::string CLIENT_DEFAULT_PATH = "client.in";
static const std::string SERVER_DEFAULT_PATH = "server.in";
static const std::string SERVER = "localhost";

// Data-only packets
struct packet {
	// Header
	int checksum;
	int len;
	int seqno;
	bool isFinal;

	// Data (Payload)
	char data[MAX_DGRAM_PAYLOAD];
};

// Ack-only packets (only 8 bytes)
struct ack_packet {
	int checksum;
	int len;
	int ackno;
};

class Utilities
{

public:
	// Split a given a string into tokens using space as a separator.
	static std::vector<std::string> split(std::string s);

	static std::string getFileName(std::string path);

	static int sendRequest(SOCKET socket, std::string message);
};