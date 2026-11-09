
#include <iostream>
#include <string>
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <cctype>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

struct Client {
    string username;
    string room;
};

map<int, Client> clients;
map<string, set<int>> rooms;
mutex serverMutex;

// Send the entire message through a TCP socket
bool sendAll(int socketFd, const string& message) {
    size_t total = 0;

    while (total < message.size()) {
        ssize_t sent = send(
            socketFd,
            message.data() + total,
            message.size() - total,
            MSG_NOSIGNAL
        );

        if (sent <= 0)
            return false;

        total += sent;
    }

    return true;
}

// Send a system message to one client.
// Caller must hold serverMutex.
void sendNotice(int clientSocket, const string& text) {
    sendAll(clientSocket, "[LAN Party] " + text + "\n");
}

// Allow simple room names without spaces
bool validRoomName(const string& name) {
    if (name.empty() || name.size() > 24)
        return false;

    for (char ch : name) {
        if (!isalnum((unsigned char)ch) &&
            ch != '_' && ch != '-')
            return false;
    }

    return true;
}

// Called while holding serverMutex
void moveClient(int clientSocket, const string& newRoom) {
    string oldRoom = clients[clientSocket].room;

    rooms[oldRoom].erase(clientSocket);

    if (oldRoom != "lobby" && rooms[oldRoom].empty())
        rooms.erase(oldRoom);

    clients[clientSocket].room = newRoom;
    rooms[newRoom].insert(clientSocket);
}

// Handle room-related commands
void handleCommand(int clientSocket, const string& command) {
    lock_guard<mutex> lock(serverMutex);

    if (command == "/rooms") {
        string result = "Available rooms: ";

        for (const auto& room : rooms) {
            result += room.first + " (" +
                      to_string(room.second.size()) + " users), ";
        }

        sendNotice(clientSocket, result);
        return;
    }

    if (command == "/leave") {
        moveClient(clientSocket, "lobby");
        sendNotice(clientSocket, "You joined lobby.");
        return;
    }

    string createPrefix = "/create ";
    string joinPrefix = "/join ";

    if (command.rfind(createPrefix, 0) == 0) {
        string roomName = command.substr(createPrefix.size());

        if (!validRoomName(roomName) || roomName == "lobby") {
            sendNotice(clientSocket, "Invalid room name.");
            return;
        }

        if (rooms.count(roomName)) {
            sendNotice(clientSocket, "Room already exists.");
            return;
        }

        rooms[roomName] = {};
        moveClient(clientSocket, roomName);

        sendNotice(clientSocket, "Created and joined #" + roomName);
        cout << "Room created: " << roomName << endl;
        return;
    }

    if (command.rfind(joinPrefix, 0) == 0) {
        string roomName = command.substr(joinPrefix.size());

        if (!rooms.count(roomName)) {
            sendNotice(clientSocket, "Room not found.");
            return;
        }

        moveClient(clientSocket, roomName);
        sendNotice(clientSocket, "Joined #" + roomName);
        return;
    }

    sendNotice(clientSocket, "Unknown command.");
}

// Forward chat messages to clients in the same room
void broadcastMessage(int sender, const string& message) {
    lock_guard<mutex> lock(serverMutex);

    string roomName = clients[sender].room;

    for (int clientSocket : rooms[roomName]) {
        if (clientSocket != sender) {
            sendAll(clientSocket, message);
        }
    }
}

// Each connected client gets its own thread
void handleClient(int clientSocket) {
    char buffer[1024];
    string pending;

    {
        lock_guard<mutex> lock(serverMutex);
        clients[clientSocket] = {"Guest", "lobby"};
        rooms["lobby"].insert(clientSocket);
    }

    cout << "Client connected and joined lobby.\n";

    while (true) {
        ssize_t bytes = recv(
            clientSocket,
            buffer,
            sizeof(buffer),
            0
        );

        if (bytes <= 0)
            break;

        pending.append(buffer, bytes);

        size_t position;

        while ((position = pending.find('\n')) != string::npos) {
            string message = pending.substr(0, position);
            pending.erase(0, position + 1);

            if (message.size() > 4096) {
                shutdown(clientSocket, SHUT_RDWR);
                break;
            }

            // The current client sends "Username: message"
            size_t separator = message.find(": ");

            if (separator == string::npos)
                continue;

            string username = message.substr(0, separator);
            string text = message.substr(separator + 2);

            if (username.empty() || username.size() > 32)
                continue;

            {
                lock_guard<mutex> lock(serverMutex);
                clients[clientSocket].username = username;
            }

            if (!text.empty() && text[0] == '/') {
                handleCommand(clientSocket, text);
            }
            else {
                cout << username << ": " << text << endl;
                broadcastMessage(
                    clientSocket,
                    username + ": " + text + "\n"
                );
            }
        }

        if (pending.size() > 4096)
            break;
    }

    {
        lock_guard<mutex> lock(serverMutex);

        string roomName = clients[clientSocket].room;
        rooms[roomName].erase(clientSocket);

        if (roomName != "lobby" && rooms[roomName].empty())
            rooms.erase(roomName);

        clients.erase(clientSocket);
    }

    close(clientSocket);
    cout << "Client disconnected.\n";
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        perror("Socket failed");
        return 1;
    }

    int option = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR,
               &option, sizeof(option));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(5000);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&address,
             sizeof(address)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    if (listen(serverSocket, 10) == -1) {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    rooms["lobby"] = {};

    cout << "LAN Party server started!\n";
    cout << "Room management enabled.\n";
    cout << "Listening on port 5000...\n";

    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);

        if (clientSocket == -1) {
            perror("Accept failed");
            continue;
        }

        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    close(serverSocket);
    return 0;
}
