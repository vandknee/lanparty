
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

int main() {
    // Create a TCP socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (serverSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    // Configure server address
    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5000);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to port 5000
    if (bind(serverSocket, (sockaddr*)&serverAddress,
             sizeof(serverAddress)) == -1) {
        perror("Bind failed");
        close(serverSocket);
        return 1;
    }

    // Wait for incoming connections
    if (listen(serverSocket, 5) == -1) {
        perror("Listen failed");
        close(serverSocket);
        return 1;
    }

    cout << "LAN Party server started on port 5000" << endl;
    cout << "Waiting for a client..." << endl;

    // Accept a connection
    int clientSocket = accept(serverSocket, nullptr, nullptr);

    if (clientSocket == -1) {
        perror("Connection failed");
        close(serverSocket);
        return 1;
    }

    
    cout << "Client connected successfully!" << endl;

    // Buffer to store incoming messages
    char buffer[1024];

    while (true) {
        // Receive data from the client
        int bytesReceived = recv(clientSocket, buffer,
                                sizeof(buffer) - 1, 0);

        if (bytesReceived <= 0) {
            cout << "Client disconnected." << endl;
            break;
        }

        // Convert received bytes into a C-style string
        buffer[bytesReceived] = '\0';

        // Print the message
        cout << "Client: " << buffer << endl;
    }

    close(clientSocket);
    close(serverSocket);

    return 0;
}
