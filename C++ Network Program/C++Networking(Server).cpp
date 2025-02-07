#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#endif

#include <iostream>
#include <cstring>
#include <vector>
#include <thread>

#define PORT 8080
#define MAX_CLIENTS 10

std::vector<int> clientSockets;

void handleClient(int clientSocket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            std::cout << "Client disconnected." << std::endl;
#ifdef _WIN32
            closesocket(clientSocket);
#else
            close(clientSocket);
#endif
            break;
        }

        std::cout << "Received: " << buffer << std::endl;

        // Broadcast the message to all other clients
        for (int otherSocket : clientSockets) {
            if (otherSocket != clientSocket) {
                send(otherSocket, buffer, bytesReceived, 0);
            }
        }
    }
}

int main() {
#ifdef _WIN32
    // Initialize Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }
#endif

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket." << std::endl;
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);

    // Bind to all available interfaces (INADDR_ANY)
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        std::cerr << "Failed to bind socket." << std::endl;
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        return 1;
    }

    if (listen(serverSocket, MAX_CLIENTS) == -1) {
        std::cerr << "Failed to listen on socket." << std::endl;
#ifdef _WIN32
        closesocket(serverSocket);
        WSACleanup();
#else
        close(serverSocket);
#endif
        return 1;
    }

    std::cout << "Server is listening on port " << PORT << "..." << std::endl;

    while (true) {
        sockaddr_in clientAddress;
        socklen_t clientAddressSize = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddress, &clientAddressSize);
        if (clientSocket == -1) {
            std::cerr << "Failed to accept client connection." << std::endl;
            continue;
        }

        std::cout << "New client connected." << std::endl;
        clientSockets.push_back(clientSocket);

        // Start a new thread to handle the client
        std::thread(handleClient, clientSocket).detach();
    }

#ifdef _WIN32
    closesocket(serverSocket);
    WSACleanup();
#else
    close(serverSocket);
#endif
    return 0;
}