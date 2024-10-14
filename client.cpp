#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

int main() {
    // Creating socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    // Specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY; // Connect to localhost; change if needed.

    // Sending connection request
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Failed to connect to the server." << endl;
        close(clientSocket); // Clean up
        return 1;
    }

    cout << "Connected to the server." << endl;

    // Loop to send Y86 instructions until "quit" or "q" is sent
    string message;
    char buffer[1024] = {0};

    while (true) {
        cout << "Enter Y86 instruction (or 'quit' to exit): ";
        getline(cin, message);

        // Check if the message is "quit" or "q"
        if (message == "quit" || message == "q") {
            cout << "Closing connection..." << endl;
            break;
        }

        // Sending data to the server
        ssize_t bytesSent = send(clientSocket, message.c_str(), message.size(), 0);
        if (bytesSent < 0) {
            cerr << "Error sending message to server." << endl;
            break;
        }

        // Receiving the server's response
        memset(buffer, 0, sizeof(buffer)); // Clear buffer before receiving
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cerr << "Error receiving message from server or server disconnected." << endl;
            break;
        }

        // Ensure the buffer is null-terminated
        buffer[bytesReceived] = '\0';

        // Print the server's response
        cout << "Server response: " << buffer << endl;
    }

    // Closing the socket
    close(clientSocket);
    cout << "Client closed." << endl;

    return 0;
}
