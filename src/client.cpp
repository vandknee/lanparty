
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <cctype>

#include <ncurses.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// Incoming messages shared between threads
queue<string> incomingMessages;
mutex messageMutex;
atomic<bool> connected{true};

// Send the entire message
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

// This thread ONLY receives network messages.
// It never directly writes to the terminal.
void receiveMessages(int socketFd) {
    char buffer[1024];
    string pending;

    while (true) {
        ssize_t bytes = recv(
            socketFd,
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
                connected = false;
                return;
            }

            lock_guard<mutex> lock(messageMutex);
            incomingMessages.push(message);
        }

        if (pending.size() > 4096)
            break;
    }

    connected = false;
}

// Display the entire chat interface
void drawScreen(
    const vector<string>& history,
    const string& input,
    const string& username,
    const string& currentRoom
){
    erase();

    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    if (rows < 7 || cols < 25) {
        mvaddstr(0, 0, "Enlarge your terminal.");
        refresh();
        return;
    }

    
    // Header
    attron(A_BOLD | COLOR_PAIR(1));
    mvaddstr(0, 2, "LAN PARTY");
    attroff(A_BOLD | COLOR_PAIR(1));

    // Connection status
    string status = connected ? "ONLINE" : "DISCONNECTED";

    if (connected)
        attron(COLOR_PAIR(2));
    else
        attron(COLOR_PAIR(3));

    mvaddnstr(0, max(2, cols - 16),
            status.c_str(), 14);

    if (connected)
        attroff(COLOR_PAIR(2));
    else
        attroff(COLOR_PAIR(3));

    // Username on the left
    string userLabel = "User: " + username;
    mvaddnstr(1, 2, userLabel.c_str(), cols - 4);

    // Current room on the right
    string roomLabel = "Room: #" + currentRoom;

    attron(COLOR_PAIR(1));

    mvaddnstr(1, max(2, cols - 34),
            roomLabel.c_str(),
            min(30, cols - 4));

    attroff(COLOR_PAIR(1));


    mvhline(2, 0, ACS_HLINE, cols);

    // Chat history
    int firstLine = 3;
    int lastLine = rows - 4;
    int availableLines = lastLine - firstLine + 1;

    int start = max(
        0,
        (int)history.size() - availableLines
    );

    int y = firstLine;

    for (int i = start; i < (int)history.size(); i++) {
        if (y > lastLine)
            break;

        const string& message = history[i];

        if (message.rfind("You: ", 0) == 0)
            attron(COLOR_PAIR(1));

        mvaddnstr(y, 2, message.c_str(), cols - 4);

        if (message.rfind("You: ", 0) == 0)
            attroff(COLOR_PAIR(1));

        y++;
    }

    // Bottom input section
    mvhline(rows - 3, 0, ACS_HLINE, cols);

    mvaddstr(rows - 2, 2, "> ");

    int inputWidth = cols - 6;

    // Show the end if the input is too long
    int offset = max(
        0,
        (int)input.size() - inputWidth
    );

    string visibleInput = input.substr(offset);

    mvaddnstr(rows - 2, 4,
              visibleInput.c_str(), inputWidth);

    mvaddnstr(
        rows - 1, 2,
        "ENTER: Send | /quit: Exit",
        cols - 4
    );

    // Place cursor at the end of the input
    move(rows - 2,
         min(cols - 2, 4 + (int)visibleInput.size()));

    refresh();
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        cout << "Usage: ./client SERVER_IP\n";
        return 1;
    }   

    string username;
    string currentRoom = "lobby";
    cout << "Enter username: ";
    getline(cin, username);

    if (username.empty() || username.size() > 24) {
        cout << "Username must be 1-24 characters.\n";
        return 1;
    }

    // Keep names compatible with our text protocol
    for (char ch : username) {
        if (!isalnum((unsigned char)ch) &&
            ch != '_' && ch != '-') {
            cout << "Use letters, numbers, _ or -.\n";
            return 1;
        }
    }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket == -1) {
        perror("Socket failed");
        return 1;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5000);

    if (inet_pton(AF_INET, argv[1],
                  &serverAddress.sin_addr) != 1) {
        cout << "Invalid IP address.\n";
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


    // Register username with the server
    string registration = username + ": /hello\n";

    if (!sendAll(clientSocket, registration)) {
        cout << "Could not register with server.\n";
        close(clientSocket);
        return 1;
    }


    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    timeout(80);
    curs_set(1);

    if (has_colors()) {
        start_color();
        use_default_colors();

        init_pair(1, COLOR_CYAN, -1);
        init_pair(2, COLOR_GREEN, -1);
        init_pair(3, COLOR_RED, -1);
    }

    // Start network receiver thread
    thread receiver(receiveMessages, clientSocket);

    vector<string> history;
    string input;

    history.push_back("Connected to LAN Party!");
    history.push_back("Type a message and press Enter.");

    bool running = true;

    while (running) {

        // Transfer newly received messages
        {
            lock_guard<mutex> lock(messageMutex);

            
            while (!incomingMessages.empty()) {
                string message = incomingMessages.front();
                incomingMessages.pop();

                string createdPrefix =
                    "[LAN Party] Created and joined #";

                string joinedPrefix =
                    "[LAN Party] Joined #";

                if (message.rfind(createdPrefix, 0) == 0) {
                    currentRoom = message.substr(createdPrefix.size());
                }
                else if (message.rfind(joinedPrefix, 0) == 0) {
                    currentRoom = message.substr(joinedPrefix.size());
                }
                else if (message == "[LAN Party] You joined lobby.") {
                    currentRoom = "lobby";
                }

                history.push_back(message);
            }

        }

        // Limit memory used for chat history
        if (history.size() > 500) {
            history.erase(
                history.begin(),
                history.begin() + (history.size() - 500)
            );
        }

        drawScreen(history, input, username, currentRoom);

        // Read keyboard without blocking forever
        int key = getch();

        if (key == ERR || key == KEY_RESIZE)
            continue;

        // Enter key
        if (key == '\n' || key == KEY_ENTER ||
            key == '\r') {

            if (input == "/quit") {
                running = false;
            }
            else if (!input.empty() && connected) {
                string outgoing =
                    username + ": " + input + "\n";

                if (!sendAll(clientSocket, outgoing)) {
                    connected = false;
                    history.push_back("Message failed to send.");
                }
                else {
                    if (input[0] != '/') {
                        history.push_back("You: " + input);
                    }
                }
            }
            else if (!connected) {
                history.push_back(
                    "Disconnected. Type /quit to exit."
                );
            }

            input.clear();
        }

        // Backspace
        else if (key == KEY_BACKSPACE ||
                 key == 127 || key == 8) {
            if (!input.empty())
                input.pop_back();
        }

        // Printable characters
        else if (key >= 32 && key <= 126) {
            if (input.size() < 1000)
                input += (char)key;
        }
    }

    // Shut down connection before joining receiver
    shutdown(clientSocket, SHUT_RDWR);
    receiver.join();

    close(clientSocket);

    // Restore normal terminal behavior
    endwin();

    cout << "Disconnected from LAN Party.\n";

    return 0;
}
