
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// Send every byte of a message
bool sendAll(int socketFd, const string& message) {
    size_t totalSent = 0;

    while (totalSent < message.size()) {
        ssize_t sent = send(
            socketFd,
            message.data() + totalSent,
            message.size() - totalSent,
            MSG_NOSIGNAL
        );

        if (sent <= 0)
            return false;

        totalSent += sent;
    }

    return true;
}

// Receive messages in a separate thread
void receiveMessages(int socketFd, atomic<bool>& connected) {
    char buffer[1024];
    string pending;

    while (true) {
        ssize_t bytesReceived = recv(
            socketFd,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytesReceived <= 0)
            break;

        pending.append(buffer, bytesReceived);

        size_t position;

        while ((position = pending.find('\n')) != string::npos) {
            string message = pending.substr(0, position);
            pending.erase(0, position + 1);

            cout << "\n" << message << endl;
        }

        if (pending.size() > 4096)
            break;
    }

    connected = false;
    cout << "\nDisconnected from server.\n";
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: ./client SERVER_IP\n";
        return 1;
    }

    string username;

    cout << "Enter your username: ";
    getline(cin, username);

    if (username.empty() || username.size() > 32 ||
        username.find('\r') != string::npos) {
        cout << "Invalid username.\n";
        return 1;
    }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5000);

    if (inet_pton(AF_INET, argv[1],
                  &serverAddress.sin_addr) != 1) {
        cout << "Invalid server IP address.\n";
        close(clientSocket);
        return 1;
    }

    if (connect(clientSocket,
                (sockaddr*)&serverAddress,
                sizeof(serverAddress)) == -1) {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    cout << "Connected to LAN Party!\n";
    cout << "Type /quit to leave.\n";

    atomic<bool> connected{true};

    thread receiver(
        receiveMessages,
        clientSocket,
        ref(connected)
    );

    string message;

    while (connected) {
        cout << "You: " << flush;

        if (!getline(cin, message) || message == "/quit")
            break;

        if (message.empty())
            continue;

        if (message.size() > 4000) {
            cout << "Message too long.\n";
            continue;
        }

        string formatted =
            username + ": " + message + "\n";

        if (!sendAll(clientSocket, formatted)) {
            cout << "Could not send message.\n";
            break;
        }
    }

    shutdown(clientSocket, SHUT_RDWR);
    receiver.join();

    close(clientSocket);
    return 0;
}
