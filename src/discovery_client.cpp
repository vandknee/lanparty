
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

vector<string> discoverServers() {
    set<string> foundServers;

    int udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    if (udpSocket == -1) {
        perror("UDP socket failed");
        return {};
    }

    int option = 1;

    if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
                   &option, sizeof(option)) == -1) {
        perror("Broadcast failed");
        close(udpSocket);
        return {};
    }

    sockaddr_in broadcastAddress{};
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(5001);

    inet_pton(AF_INET, "255.255.255.255",
              &broadcastAddress.sin_addr);

    string request = "LANPARTY_DISCOVER";

    // Search for servers on the local network
    ssize_t sent = sendto(
        udpSocket,
        request.c_str(),
        request.size(),
        0,
        (sockaddr*)&broadcastAddress,
        sizeof(broadcastAddress)
    );

    if (sent < 0)
        perror("UDP broadcast failed");

    // Also search the same computer
    sockaddr_in localAddress{};
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(5001);

    inet_pton(AF_INET, "127.0.0.1",
              &localAddress.sin_addr);

    sendto(
        udpSocket,
        request.c_str(),
        request.size(),
        0,
        (sockaddr*)&localAddress,
        sizeof(localAddress)
    );

    auto deadline = chrono::steady_clock::now()
                    + chrono::seconds(3);

    while (chrono::steady_clock::now() < deadline) {
        auto remaining = chrono::duration_cast<
            chrono::microseconds
        >(deadline - chrono::steady_clock::now());

        if (remaining.count() <= 0)
            break;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(udpSocket, &readSet);

        timeval waitTime{};
        waitTime.tv_sec = remaining.count() / 1000000;
        waitTime.tv_usec = remaining.count() % 1000000;

        int ready = select(
            udpSocket + 1,
            &readSet,
            nullptr,
            nullptr,
            &waitTime
        );

        if (ready <= 0)
            break;

        char buffer[1024];
        sockaddr_in serverAddress{};
        socklen_t addressLength = sizeof(serverAddress);

        ssize_t bytes = recvfrom(
            udpSocket,
            buffer,
            sizeof(buffer),
            0,
            (sockaddr*)&serverAddress,
            &addressLength
        );

        if (bytes <= 0)
            continue;

        string response(buffer, bytes);

        if (response != "LANPARTY_SERVER|5000")
            continue;

        char serverIP[INET_ADDRSTRLEN];

        if (inet_ntop(
                AF_INET,
                &serverAddress.sin_addr,
                serverIP,
                sizeof(serverIP)
            ) == nullptr) {
            continue;
        }

        foundServers.insert(serverIP);
    }

    close(udpSocket);

    return vector<string>(
        foundServers.begin(),
        foundServers.end()
    );
}

// Allows independent testing of discovery
#ifdef DISCOVERY_STANDALONE

int main() {
    cout << "Searching for LAN Party servers...\n";

    vector<string> servers = discoverServers();

    if (servers.empty()) {
        cout << "No servers found.\n";
    }
    else {
        for (int i = 0; i < (int)servers.size(); i++) {
            cout << i + 1 << ". " << servers[i]
                 << " (Port 5000)\n";
        }
    }

    return 0;
}

#endif
