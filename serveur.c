#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 1975
#define MAX_CLIENTS 10
#define BUF_SIZE 1024

typedef struct {
    int sock;
    char name[BUF_SIZE];
    int active;
} Client;

Client clients[MAX_CLIENTS];

// Function prototypes
int init_server_socket();
void handle_client_message(int client_index);
void broadcast_message(const char *message, int sender_index);
void send_message_to_client(int client_index, const char *message);
void remove_client(int index);
int find_client_by_name(const char *name);

int main() {
    int server_sock = init_server_socket();
    fd_set read_fds;
    int max_fd = server_sock > STDIN_FILENO ? server_sock : STDIN_FILENO;

    // Initialize client sockets to -1 (indicating empty slots)
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = -1;
    }

    printf("Enter a message to broadcast or target a specific client (e.g., 'Client0:Hello'):\n");

    // Main server loop
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_sock, &read_fds); // Monitor new client connections
        FD_SET(STDIN_FILENO, &read_fds); // Monitor server console input

        // Add all active client sockets to the read set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &read_fds);
                if (clients[i].sock > max_fd) {
                    max_fd = clients[i].sock;
                }
            }
        }

        // Use select to wait for activity on any socket or stdin
        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("select() error");
            continue;
        }

        // Check for new incoming connections on the server socket
        if (FD_ISSET(server_sock, &read_fds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int new_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);

            if (new_sock < 0) {
                perror("accept() error");
                continue;
            }

            // Add new client to the clients array
            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sock == -1) {
                    clients[i].sock = new_sock;
                    clients[i].active = 1;
                    snprintf(clients[i].name, BUF_SIZE, "Client%d", i);
                    printf("New client connected: %s\n", clients[i].name);
                    added = 1;
                    break;
                }
            }

            if (!added) {
                printf("Max clients reached. Connection refused.\n");
                close(new_sock);
            }
        }

        // Check for server console input
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char buffer[BUF_SIZE];
            memset(buffer, 0, BUF_SIZE);

            if (fgets(buffer, BUF_SIZE, stdin) != NULL) {
                buffer[strcspn(buffer, "\n")] = '\0'; // Remove trailing newline

                // Parse the input for targeted or broadcast message
                char *delimiter = strchr(buffer, ':');
                if (delimiter) {
                    *delimiter = '\0'; // Split the string into target and message
                    const char *target = buffer;
                    const char *message = delimiter + 1;

                    int client_index = find_client_by_name(target);
                    if (client_index != -1) {
                        send_message_to_client(client_index, message);
                    } else {
                        printf("Client %s not found.\n", target);
                    }
                } else {
                    // Broadcast the message if no target specified
                    printf("Broadcasting: %s\n", buffer);
                    broadcast_message(buffer, -1); // Server sends to all clients
                }
            }
        }

        // Handle messages from each client
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0 && FD_ISSET(clients[i].sock, &read_fds)) {
                handle_client_message(i); // Process message from the client
            }
        }
    }

    close(server_sock);
    return 0;
}

// Function to initialize the server socket
int init_server_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() error");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (listen(sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);
    return sock;
}

// Function to handle messages from a specific client
void handle_client_message(int client_index) {
    char buffer[BUF_SIZE];
    memset(buffer, 0, BUF_SIZE); // Clear buffer before receiving

    int n = recv(clients[client_index].sock, buffer, BUF_SIZE - 1, 0);

    if (n <= 0) {
        printf("Client %s disconnected.\n", clients[client_index].name);
        remove_client(client_index);
    } else {
        buffer[n] = '\0';
        printf("Message from %s: %s\n", clients[client_index].name, buffer);

        // Broadcast the message to all other clients
        broadcast_message(buffer, client_index);
    }
}

// Function to send a message to a specific client
void send_message_to_client(int client_index, const char *message) {
    if (clients[client_index].sock <= 0) {
        printf("Client %s is not active. Message not sent.\n", clients[client_index].name);
        return;
    }

    if (send(clients[client_index].sock, message, strlen(message), 0) < 0) {
        perror("Failed to send message to client");
    } else {
        printf("Message sent to %s: %s\n", clients[client_index].name, message);
    }
}

// Function to broadcast a message to all clients
void broadcast_message(const char *message, int sender_index) {
    char full_message[BUF_SIZE];
    memset(full_message, 0, BUF_SIZE);

    if (sender_index >= 0) {
        strcpy(full_message, "[");
        strcat(full_message, clients[sender_index].name);
        strcat(full_message, "]: ");
    } else {
        strcpy(full_message, "[Server]: ");
    }
    strcat(full_message, message);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && i != sender_index) {
            send_message_to_client(i, full_message);
        }
    }
}

// Function to find a client by name
int find_client_by_name(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && strcmp(clients[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to remove a client from the clients array
void remove_client(int index) {
    close(clients[index].sock);
    clients[index].sock = -1;
    clients[index].active = 0;
    printf("Client %s removed.\n", clients[index].name);
}




