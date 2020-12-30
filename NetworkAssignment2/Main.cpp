#include "Server.h"
#include "Client.h"
#include "Utilities.h"

#define SERVER_HOST "server"
#define CLIENT_HOST "client"

int main()
{

    std::cout << sizeof(struct packet) << std::endl;
    std::cout << sizeof(struct ack_packet) << std::endl;
    std::string host;
    getline(std::cin, host);

    if (host == SERVER_HOST) {
        Server server = Server();
        server.run();
    } else if (host == CLIENT_HOST) {
        Client client = Client();
        client.execute();
    }

    std::cout << "Program finished" << std::endl;
    std::cin.get();
    return 0;
}