
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

void runDiscoveryServer() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        perror("UDP socket failed");
        return;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(5001);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&address,
             sizeof(address)) == -1) {
        perror("UDP bind failed");
        close(udpSocket);
        return;
    }

    cout << "UDP discovery running on port 5001\n";

    char buffer[1024];

    while (true) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);

        ssize_t bytes = recvfrom(
            udpSocket,
            buffer,
            sizeof(buffer),
            0,
            (sockaddr*)&clientAddress,
            &clientLength
        );

        if (bytes <= 0)
            continue;

        string request(buffer, bytes);

        if (request != "LANPARTY_DISCOVER")
            continue;

        string response = "LANPARTY_SERVER|5000";

        ssize_t sent = sendto(
            udpSocket,
            response.c_str(),
            response.size(),
            0,
            (sockaddr*)&clientAddress,
            clientLength
        );

        if (sent < 0) {
            perror("UDP response failed");
            continue;
        }

        char clientIP[INET_ADDRSTRLEN];

        inet_ntop(
            AF_INET,
            &clientAddress.sin_addr,
            clientIP,
            sizeof(clientIP)
        );

        cout << "Discovery request answered: "
             << clientIP << endl;
    }

    close(udpSocket);
}
