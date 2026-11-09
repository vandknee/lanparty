
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {
    // Create client socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        perror("Socket creation failed");
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5000);

    // Connect to server on the same computer
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    cout << "Connecting to LAN Party server..." << endl;

    if (connect(clientSocket, (sockaddr*)&serverAddress,
                sizeof(serverAddress)) == -1) {
        perror("Connection failed");
        close(clientSocket);
        return 1;
    }

    
    cout << "Connected successfully!" << endl;

    string message;

    while (true) {
        cout << "You: ";
        getline(cin, message);

        if (!cin || message == "/quit") {
            break;
        }

        // Send the message to the server
        if (send(clientSocket, message.c_str(),
                message.size(), 0) == -1) {
            perror("Send failed");
            break;
        }
    }

    close(clientSocket);


    return 0;
}
