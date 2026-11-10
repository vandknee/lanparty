
#include <iostream>
#include <string>
#include <set>
#include <chrono>
#include <sys/socket.h>
#include <sys/select.h>
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

    // Allow UDP broadcasts
    int option = 1;

    if (setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST,
                   &option, sizeof(option)) == -1) {
        perror("Broadcast option failed");
        close(udpSocket);
        return 1;
    }

    sockaddr_in broadcastAddress{};
    broadcastAddress.sin_family = AF_INET;
    broadcastAddress.sin_port = htons(5001);

    inet_pton(
        AF_INET,
        "255.255.255.255",
        &broadcastAddress.sin_addr
    );

    string request = "LANPARTY_DISCOVER";

    // Broadcast across the local network
    ssize_t sent = sendto(
        udpSocket,
        request.c_str(),
        request.size(),
        0,
        (sockaddr*)&broadcastAddress,
        sizeof(broadcastAddress)
    );

    if (sent == -1)
        perror("Broadcast failed");

    // Also test the local machine directly
    sockaddr_in localAddress{};
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(5001);

    inet_pton(
        AF_INET,
        "127.0.0.1",
        &localAddress.sin_addr
    );

    sendto(
        udpSocket,
        request.c_str(),
        request.size(),
        0,
        (sockaddr*)&localAddress,
        sizeof(localAddress)
    );

    cout << "Searching for LAN Party servers...\n";

    set<string> foundServers;

    auto deadline =
        chrono::steady_clock::now() +
        chrono::seconds(3);

    while (chrono::steady_clock::now() < deadline) {
        auto remaining = chrono::duration_cast<
            chrono::microseconds
        >(deadline - chrono::steady_clock::now());

        if (remaining.count() <= 0)
            break;

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(udpSocket, &readSet);

        timeval timeout{};
        timeout.tv_sec = remaining.count() / 1000000;
        timeout.tv_usec = remaining.count() % 1000000;

        int ready = select(
            udpSocket + 1,
            &readSet,
            nullptr,
            nullptr,
            &timeout
        );

        if (ready < 0) {
            perror("select failed");
            break;
        }

        if (ready == 0)
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

        inet_ntop(
            AF_INET,
            &serverAddress.sin_addr,
            serverIP,
            sizeof(serverIP)
        );

        foundServers.insert(serverIP);
    }

    cout << "\nDiscovery complete!\n";

    if (foundServers.empty()) {
        cout << "No LAN Party servers found.\n";
    }
    else {
        int number = 1;

        for (const string& ip : foundServers) {
            cout << number++ << ". " << ip
                 << " (TCP port 5000)\n";
        }
    }

    close(udpSocket);
    return 0;
}
