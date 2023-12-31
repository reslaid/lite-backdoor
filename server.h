#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#define PORT 8080
#define MAX_BUFFER_SIZE 1024
#define MAX_CLIENTS 10

struct ClientInfo {
    int socket;
    char key[64];
};

struct Server {
    struct ClientInfo connected[MAX_CLIENTS];
    struct sockaddr_in serverAddr, clientAddr;
#ifdef _WIN32
    WSADATA wsaData;
    SOCKET globalSocket;
#else
    int globalSocket;
#endif
};

#ifdef _WIN32
DWORD WINAPI HandleClientCommands(LPVOID lpParam) {
#else
void *HandleClientCommands(void *lpParam) {
#endif
    struct ClientInfo *client = (struct ClientInfo *)lpParam;

    char buffer[MAX_BUFFER_SIZE];
    int bytesRead;

    while (1) {
        bytesRead = recv(client->socket, buffer, sizeof(buffer), 0);
        if (bytesRead <= 0) {
            break;
        }

        buffer[bytesRead] = '\0';

        printf("Received from client %s: %s\n", client->key, buffer);

        char response[MAX_BUFFER_SIZE];
        snprintf(response, sizeof(response), "Server received: %s", buffer);

        send(client->socket, response, strlen(response), 0);
    }

    printf("Client %s disconnected\n", client->key);
#ifdef _WIN32
    closesocket(client->socket);
#else
    close(client->socket);
#endif
    client->socket = 0;

#ifdef _WIN32
    return 0;
#else
    pthread_exit(NULL);
#endif
}

int SendRequestToClient(struct Server *server, int clientIndex, const char *request) {
    if (clientIndex < 0 || clientIndex >= MAX_CLIENTS || server->connected[clientIndex].socket == 0) {
        fprintf(stderr, "Invalid client index\n");
        return -1;
    }

    const struct ClientInfo *client = &server->connected[clientIndex];
    return send(client->socket, request, strlen(request), 0);
}

void InitServer(struct Server *server) {
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &server->wsaData) != 0) {
        fprintf(stderr, "Failed to initialize Winsock\n");
        return;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        fprintf(stderr, "Socket creation failed\n");
        WSACleanup();
        return;
    }

    server->globalSocket = serverSocket;
#else
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        fprintf(stderr, "Socket creation failed\n");
        return;
    }
#endif

    server->serverAddr.sin_family = AF_INET;
    server->serverAddr.sin_addr.s_addr = INADDR_ANY;
    server->serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr *)&server->serverAddr, sizeof(server->serverAddr)) == -1) {
        fprintf(stderr, "Bind failed\n");
#ifdef _WIN32
        closesocket(serverSocket);
#else
        close(serverSocket);
#endif
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    if (listen(serverSocket, SOMAXCONN) == -1) {
        fprintf(stderr, "Listen failed\n");
#ifdef _WIN32
        closesocket(serverSocket);
#else
        close(serverSocket);
#endif
#ifdef _WIN32
        WSACleanup();
#endif
        return;
    }

    printf("Server listening on 0.0.0.0:%d\n", PORT);

    while (1) {
        int clientSocket;
        int addrLen = sizeof(server->clientAddr);

        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&server->clientAddr, &addrLen)) == -1) {
            fprintf(stderr, "Accept failed\n");
#ifdef _WIN32
            closesocket(serverSocket);
#else
            close(serverSocket);
#endif
#ifdef _WIN32
            WSACleanup();
#endif
            return;
        }

        char clientIP[INET_ADDRSTRLEN];
#ifdef _WIN32
        InetNtop(AF_INET, &(server->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
#else
        inet_ntop(AF_INET, &(server->clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
#endif
        printf("[%ld] Connection from %s\n", (long)time(0), clientIP);

        char clientKey[64];
        snprintf(clientKey, sizeof(clientKey), "Client%d", MAX_CLIENTS - serverSocket);

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (server->connected[i].socket == 0) {
                server->connected[i].socket = clientSocket;
                strncpy(server->connected[i].key, clientKey, sizeof(server->connected[i].key));

#ifdef _WIN32
                HANDLE thread = CreateThread(NULL, 0, HandleClientCommands, &server->connected[i], 0, NULL);
                CloseHandle(thread);
#else
                pthread_t thread;
                pthread_create(&thread, NULL, HandleClientCommands, &server->connected[i]);
                pthread_detach(thread);
#endif
                break;
            }
        }
    }
}
