#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <Ws2tcpip.h>
#include <winsock2.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

struct ClientInfo {
    SOCKET socket;
    std::string key;
};

void HandleClientCommands(ClientInfo& client) {
    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;

    while (true) {
        bytesRead = recv(client.socket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';

        std::string command(buffer);
        std::cout << "Received from client " << client.key << ": " << command << std::endl;

        std::string response = "Server received: " + command;
        send(client.socket, response.c_str(), response.size(), 0);
    }

    std::cout << "Client " << client.key << " disconnected" << std::endl;
}

class Server {
private:
    std::vector<ClientInfo> connected;
    sockaddr_in serverAddr, clientAddr;
    WSADATA wsaData;
    SOCKET globalSocket;

public:
    Server() {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "Failed to initialize Winsock" << std::endl;
            return;
        }

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            WSACleanup();
            return;
        }

        globalSocket = serverSocket;

        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(PORT);

        if (bind(serverSocket, reinterpret_cast<SOCKADDR*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed" << std::endl;
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        std::cout << "Server listening on 0.0.0.0:" << PORT << std::endl;

        while (true) {
            SOCKET clientSocket;
            int addrLen = sizeof(clientAddr);

            if ((clientSocket = accept(serverSocket, reinterpret_cast<SOCKADDR*>(&clientAddr), &addrLen)) == INVALID_SOCKET) {
                std::cerr << "Accept failed" << std::endl;
                closesocket(serverSocket);
                WSACleanup();
                return;
            }

            char clientIP[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
            std::cout << "[" << std::time(0) << "] Connection from " << clientIP << std::endl;

            std::string clientKey = "Client" + std::to_string(connected.size() + 1);

            connected.push_back({ clientSocket, clientKey });

            std::cout << "Connected clients:" << std::endl;
            for (size_t i = 0; i < connected.size(); ++i) {
                const auto& client = connected[i];
                std::cout << "  " << i + 1 << ": " << client.key << ": " << clientIP << std::endl;
            }

            std::thread(HandleClientCommands, std::ref(connected.back())).detach();
        }
    }

    ~Server() {
        closesocket(globalSocket);
        WSACleanup();
    }

    int SendRequestToClient(int clientIndex, const std::string& request) {
        if (clientIndex < 0 || clientIndex >= connected.size()) {
            std::cerr << "Invalid client index" << std::endl;
            return;
        }

        const auto& client = connected[clientIndex];
        return send(client.socket, request.c_str(), request.size(), 0);
    }
};
