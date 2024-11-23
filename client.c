#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
#endif

#define SERVER_PORT 1977
#define BUF_SIZE 1024

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

int main() {
#ifdef WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "Winsock initialization failed.\n");
        return 1;
    }
#endif

    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    SOCKADDR_IN server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    printf("Connecting to server...\n");
    if (connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        perror("Connection failed");
        closesocket(sock);
        return EXIT_FAILURE;
    }

    printf("Connected to server.\n");

    char buffer[BUF_SIZE];
    fd_set rdfs;

    while (1) {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) < 0) {
            perror("select() error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &rdfs)) {
            fgets(buffer, BUF_SIZE, stdin);
            buffer[strcspn(buffer, "\n")] = '\0';

            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("send() error");
                break;
            }

            if (strcmp(buffer, "EXIT") == 0) {
                printf("Disconnecting from server...\n");
                break;
            }
        }

        if (FD_ISSET(sock, &rdfs)) {
            int n = recv(sock, buffer, BUF_SIZE - 1, 0);
            if (n <= 0) {
                printf("Server disconnected.\n");
                break;
            }

            buffer[n] = '\0';
            printf("%s\n", buffer);
        }
    }

    closesocket(sock);

#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}
