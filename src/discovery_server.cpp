
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {
    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5001);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(udpSocket, (sockaddr*)&serverAddress,
             sizeof(serverAddress)) == -1) {
        perror("Bind failed");
        close(udpSocket);
        return 1;
    }

    cout << "LAN Party UDP Discovery Server started!\n";
    cout << "Listening for discovery requests on port 5001...\n";

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

        if (request == "LANPARTY_DISCOVER") {
            char clientIP[INET_ADDRSTRLEN];

            inet_ntop(
                AF_INET,
                &clientAddress.sin_addr,
                clientIP,
                sizeof(clientIP)
            );

            cout << "Discovery request from "
                 << clientIP << endl;

            string response = "LANPARTY_SERVER|5000";

            sendto(
                udpSocket,
                response.c_str(),
                response.size(),
                0,
                (sockaddr*)&clientAddress,
                clientLength
            );

            cout << "Discovery response sent.\n";
        }
    }

    close(udpSocket);
    return 0;
}
