
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

vector<int> clients;
mutex clientsMutex;

// Send the complete message
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

// Forward a message to other connected clients
void broadcastMessage(const string& message, int sender) {
    lock_guard<mutex> lock(clientsMutex);

    for (int client : clients) {
        if (client != sender) {
            sendAll(client, message);
        }
    }
}

// Each client gets its own thread
void handleClient(int clientSocket) {
    char buffer[1024];
    string pending;

    cout << "A client connected.\n";

    while (true) {
        ssize_t bytesReceived = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytesReceived <= 0)
            break;

        pending.append(buffer, bytesReceived);

        size_t position;

        while ((position = pending.find('\n')) != string::npos) {
            string message = pending.substr(0, position + 1);
            pending.erase(0, position + 1);

            if (message.size() > 4096) {
                shutdown(clientSocket, SHUT_RDWR);
                break;
            }

            cout << message << flush;

            broadcastMessage(message, clientSocket);
        }

        if (pending.size() > 4096)
            break;
    }

    {
        lock_guard<mutex> lock(clientsMutex);

        clients.erase(
            remove(clients.begin(), clients.end(), clientSocket),
            clients.end()
        );
    }

    close(clientSocket);

    cout << "A client disconnected.\n";
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    int option = 1;
    setsockopt(
        serverSocket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &option,
        sizeof(option)
    );

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket,
             (sockaddr*)&serverAddress,
             sizeof(serverAddress)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 10) == -1) {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    cout << "LAN Party server started!\n";
    cout << "Listening on port 5000...\n";

    while (true) {
        int clientSocket = accept(
            serverSocket, nullptr, nullptr
        );

        if (clientSocket == -1) {
            perror("Accept failed");
            continue;
        }

        {
            lock_guard<mutex> lock(clientsMutex);
            clients.push_back(clientSocket);
        }

        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
