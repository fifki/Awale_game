#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 1975
#define BUF_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_address;
    char buffer[BUF_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully.\n");

    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_address.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Set socket to non-blocking mode
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // Set up select file descriptors
    fd_set read_fds;
    int max_fd = sock > STDIN_FILENO ? sock : STDIN_FILENO;

    printf("Enter message (or 'exit' to quit): ");

    // Communication loop
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock, &read_fds);        // Monitor server messages
        FD_SET(STDIN_FILENO, &read_fds); // Monitor user input

        // Use select to wait for activity on either stdin or socket
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select() error");
            break;
        }

        // Check if there's user input to send
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            memset(buffer, 0, BUF_SIZE);
            if (fgets(buffer, BUF_SIZE, stdin) == NULL) {
                perror("Input error");
                break;
            }

            // Check for exit command
            if (strncmp(buffer, "exit", 4) == 0) {
                printf("Exiting...\n");
                break;
            }

            // Send message to server
            if (send(sock, buffer, strlen(buffer), 0) < 0) {
                perror("Send failed");
                break;
            }
        }

        // Check if there's a message from the server
        if (FD_ISSET(sock, &read_fds)) {
            memset(buffer, 0, BUF_SIZE);
            int n = recv(sock, buffer, BUF_SIZE - 1, 0);
            if (n <= 0) {
                printf("Server disconnected.\n");
                break;
            }
            buffer[n] = '\0';
            printf("Server: %s\n", buffer);
            printf("Enter message (or 'exit' to quit): ");
            fflush(stdout);
        }
    }

    // Clean up and close socket
    close(sock);
    return 0;
}
