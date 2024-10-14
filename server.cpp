#include <cstring>
#include <iostream>
#include <memory> // For smart pointers
#include "y86_instruction_handler.h"
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
#include <sys/wait.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

// Signal handler to clean up child processes
void handle_sigchld(int sig) {
    while (waitpid(-1, nullptr, WNOHANG) > 0); // Clean up any terminated child processes
}

// Global map to store each client's instruction handler, indexed by the client socket
unordered_map<int, shared_ptr<y86_instruction_handler>> client_lists;

// Function to process the client's command and modify their list
string process_command(int clientSocket, const string& command) {
    auto handler_copy = client_lists[clientSocket];
    return handler_copy->handle_instruction(const_cast<string&>(command)); // Avoid copying
}

int main() {
    // Register signal handler for SIGCHLD to avoid zombie processes
    signal(SIGCHLD, handle_sigchld);

    // Creating socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // Specifying the address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Binding socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to bind the socket." << endl;
        close(serverSocket); // Clean up
        //comment
        return 1;
    } 

    // Listening to the assigned socket
    if (listen(serverSocket, 5) == -1) {
        cerr << "Failed to listen on the socket." << endl;
        close(serverSocket); // Clean up
        return 1;
    }

    cout << "Server is running and waiting for connections..." << endl;

    // Loop to accept multiple clients
    while (true) {
        // Accepting connection request
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == -1) {
            cerr << "Failed to accept connection." << endl;
            continue;
        }

        // Create a shared_ptr for each client handler
        client_lists[clientSocket] = make_shared<y86_instruction_handler>();

        // Fork a new process to handle the client
        pid_t pid = fork();
        if (pid == -1) {
            cerr << "Failed to fork process." << endl;
            close(clientSocket);
            continue;
        }

        if (pid == 0) {  // Child process
            close(serverSocket); // Child doesn't need access to the main server socket

            char buffer[1024] = {0};
            while (true) {
                // Clear buffer before each receive
                memset(buffer, 0, sizeof(buffer));

                // Receiving data
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
                if (bytesReceived <= 0) {
                    cout << "Client disconnected." << endl;
                    break;
                }

                // Process the command sent by the client
                string command(buffer, bytesReceived);  // Create string from buffer
                string response = process_command(clientSocket, command);

                // Send the response back to the client
                send(clientSocket, response.c_str(), response.size(), 0);
            }

            // Close client socket and exit child process
            close(clientSocket);
            cout << "Client process closed." << endl;
            exit(0);
        } else {  // Parent process
            close(clientSocket); // Parent doesn't need the client's socket, only child does
        }
    }

    // Closing the server socket (in case we ever exit the loop)
    close(serverSocket);
    return 0;
}
