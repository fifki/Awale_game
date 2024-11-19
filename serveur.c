#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "server2.h"
#include "rules.h"

#define PORT 1975
#define MAX_CLIENTS 10
#define BUF_SIZE 1024

// Function declarations


typedef struct {
    int sock;
    char pseudoname[MAX_PSEUDO_LENGTH];
    int id;
    int active; // Whether the client is in a game or available
} Client;

typedef struct {
    int client1_id;
    int client2_id;
    Plateau plateau; // Game state for this session
} GameSession;

typedef struct {
    int sender_id;
    int receiver_id;
} PendingInvite;


Client clients[MAX_CLIENTS];
GameSession games[MAX_CLIENTS / 2]; // Max simultaneous games
PendingInvite pending_invites[MAX_CLIENTS];


int main() {
    int server_sock = init_server_socket();
    fd_set rdfs;
    int max_sock = server_sock;

    // Initialize clients and games
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = -1;
        clients[i].active = 0;
        if (i < MAX_CLIENTS / 2) {
            games[i].client1_id = -1;
            games[i].client2_id = -1;
        }
    }
      // Initialize pending invitations
    for (int i = 0; i < MAX_CLIENTS; i++) {
        pending_invites[i].sender_id = -1;
        pending_invites[i].receiver_id = -1;
    }

    while (1) {
        FD_ZERO(&rdfs);
        FD_SET(server_sock, &rdfs);

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0) {
                FD_SET(clients[i].sock, &rdfs);
                if (clients[i].sock > max_sock) {
                    max_sock = clients[i].sock;
                }
            }
        }

        if (select(max_sock + 1, &rdfs, NULL, NULL, NULL) < 0) {
            perror("select() error");
            continue;
        }

        if (FD_ISSET(server_sock, &rdfs)) {
            handle_new_client(server_sock);
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sock > 0 && FD_ISSET(clients[i].sock, &rdfs)) {
                handle_client_message(i);
            }
        }
    }

    close(server_sock);
    return 0;
}

int init_server_socket() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
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

void handle_new_client(int server_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int new_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);

    if (new_sock < 0) {
        perror("accept() error");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == -1) {
            clients[i].sock = new_sock;
            clients[i].id = rand() % 10000;
            //clients[i].active = 1;
            printf("New client connected: socket=%d, id=%d\n", new_sock, clients[i].id);
            send_to_client(new_sock, "Welcome! Enter your pseudoname: ");
            return;
        }
    }

    printf("Max clients reached. Connection refused.\n");
    close(new_sock);
}

void handle_client_message(int client_index) {
    char buffer[BUF_SIZE];
    int n = recv(clients[client_index].sock, buffer, BUF_SIZE - 1, 0);

    if (n <= 0) {
        printf("Client %d disconnected.\n", clients[client_index].id);
        remove_client(client_index);
        return;
    }

    buffer[n] = '\0';
    printf("Message from %s: %s\n", clients[client_index].pseudoname, buffer);

    if (strncmp(buffer, "INVITE", 6) == 0) {
        int target_id = atoi(buffer + 7);
        handle_game_invitation(client_index, target_id);
    } else if (strncmp(buffer, "ACCEPT", 6) == 0) {
        handle_accept_command(client_index);
    } else if (strncmp(buffer, "DECLINE", 7) == 0) {
        handle_decline_command(client_index);
    } else if (strncmp(buffer, "MOVE", 4) == 0) {
        int game_index = find_game_by_client_id(clients[client_index].id);
        int move = atoi(buffer + 5);
        handle_game_move(game_index, clients[client_index].id, move);
    } else if (clients[client_index].pseudoname[0] == '\0') {
        strncpy(clients[client_index].pseudoname, buffer, MAX_PSEUDO_LENGTH - 1);
        clients[client_index].pseudoname[MAX_PSEUDO_LENGTH - 1] = '\0'; // Ensure null termination
        printf("Client %d set pseudoname: %s\n", clients[client_index].id, clients[client_index].pseudoname);
        broadcast_clients_list();
    }
}

void broadcast_clients_list() {
    char list[BUF_SIZE] = "Connected clients:\n";

    // Build the list of clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0) { // Include all connected clients in the list
            char entry[128];
            snprintf(entry, sizeof(entry), "ID: %d, Name: %s\n", clients[i].id, clients[i].pseudoname);
            strncat(list, entry, sizeof(list) - strlen(list) - 1);
        }
    }

    // Send the list to only non-active clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ((clients[i].sock > 0) && (!clients[i].active)) { // Exclude active players
            send_to_client(clients[i].sock, list);
        }
    }
}

void send_to_client(int client_sock, const char* message) {
    if (client_sock < 0) {
        printf("Error: Invalid socket descriptor.\n");
        return;
    }
    if (send(client_sock, message, strlen(message), 0) < 0) {
        perror("send() failed");
    }
}

void remove_client(int index) {
    if (clients[index].active) {
        // Find the game session involving this client
        for (int i = 0; i < MAX_CLIENTS / 2; i++) {
            if (games[i].client1_id == clients[index].id || games[i].client2_id == clients[index].id) {
                // Mark the other client as not active
                int other_index;
                if (games[i].client1_id == clients[index].id) {
                    other_index = find_client_by_id(games[i].client2_id);
                } else {
                    other_index = find_client_by_id(games[i].client1_id);
                }

                if (other_index != -1) {
                    clients[other_index].active = 0;
                }

                // Reset the game session
                games[i].client1_id = -1;
                games[i].client2_id = -1;
                break;
            }
        }
    }

    close(clients[index].sock);
    clients[index].sock = -1;
    clients[index].active = 0; // Ensure client is marked as not active
    clients[index].pseudoname[0] = '\0';
    printf("Client %d removed.\n", clients[index].id);
    broadcast_clients_list();
}

int create_game_session(int client1_id, int client2_id) {
    int client1_index = -1, client2_index = -1;

    clients[client1_index].active = 1; 
    clients[client2_index].active = 1;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].id == client1_id) client1_index = i;
        if (clients[i].id == client2_id) client2_index = i;
    }

    if (client1_index == -1 || client2_index == -1) {
        printf("Invalid client IDs: %d or %d\n", client1_id, client2_id);
        return -1;
    }

    for (int i = 0; i < MAX_CLIENTS / 2; i++) {
        if (games[i].client1_id == -1) {
            games[i].client1_id = client1_id;
            games[i].client2_id = client2_id;

            // Validate pseudonyms
            if (clients[client1_index].pseudoname[0] == '\0' || 
                clients[client2_index].pseudoname[0] == '\0') {
                printf("Error: Empty pseudoname for one or both clients.\n");
                return -1;
            }

            printf("Initializing game board for session %d\n", i);
            initialiser_plateau(&games[i].plateau);

            strcpy(games[i].plateau.pseudo_joueur1, clients[client1_index].pseudoname);
            strcpy(games[i].plateau.pseudo_joueur2, clients[client2_index].pseudoname);

            printf("Game session created between %d and %d\n", client1_id, client2_id);
            return i;
        }
    }

    printf("No available game slots.\n");
    return -1;
}

void handle_game_invitation(int sender_id, int target_id) {
    if (clients[sender_id].id == target_id) {
        send_to_client(clients[sender_id].sock, "You cannot invite yourself.\n");
        return;
    }
     // Check if the target client is already in a game
    // Check if the target client is already in a game
    int target_index = -1;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].id == target_id) {
            target_index = i;
            break;
        }
    }

    if (target_index == -1 || clients[target_index].active) {
        send_to_client(clients[sender_id].sock, "The target client is unavailable (already in a game).\n");
        return;
    }

    // Check if the target is already in a game
    if (clients[target_index].active) {
        send_to_client(clients[sender_id].sock, "The target client is unavailable (already in a game).\n");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pending_invites[i].sender_id == -1 && pending_invites[i].receiver_id == -1) {
            pending_invites[i].sender_id = clients[sender_id].id;
            pending_invites[i].receiver_id = target_id;

            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].id == target_id && clients[j].sock > 0) {
                    char invite_msg[BUF_SIZE];
                    snprintf(invite_msg, BUF_SIZE, "INVITE from %s (ID: %d). Accept? (yes/no)", 
                        clients[sender_id].pseudoname, clients[sender_id].id);
                    send_to_client(clients[j].sock, invite_msg);
                    return;
                }
            }

            // If target client not found
            send_to_client(clients[sender_id].sock, "Target client not found.\n");
            pending_invites[i].sender_id = -1;
            pending_invites[i].receiver_id = -1;
            return;
        }
    }

    send_to_client(clients[sender_id].sock, "No space for new invitations.\n");
}

void handle_accept_command(int receiver_index) {
    int receiver_id = clients[receiver_index].id;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pending_invites[i].receiver_id == receiver_id) {
            int sender_id = pending_invites[i].sender_id;

            if (sender_id == -1 || receiver_id == -1) {
                printf("Invalid sender or receiver IDs in pending_invites.\n");
                return;
            }

            // Find sender_index
            int sender_index = -1;
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].id == sender_id) {
                    sender_index = j;
                    break;
                }
            }

            if (sender_index == -1) {
                printf("Error: Sender not found.\n");
                send_to_client(clients[receiver_index].sock, "Error: Sender not found.\n");
                return;
            }

            // Validate sockets
            if (clients[sender_index].sock < 0 || clients[receiver_index].sock < 0) {
                printf("Error: One or both client sockets are invalid.\n");
                return;
            }

            // Create the game session
            int game_index = create_game_session(sender_id, receiver_id);

            if (game_index != -1) {
                char msg[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "Game accepted! You can now play.\n");

                // Notify both clients
                send_to_client(clients[sender_index].sock, msg);
                send_to_client(clients[receiver_index].sock, msg);

                // Send initial board
                Plateau *p = &games[game_index].plateau;
                char board_msg[BUF_SIZE];
                snprintf(board_msg, BUF_SIZE,
                         "Game starts! Initial board:\nPlayer 1 (%s): %d\nPlayer 2 (%s): %d\nBoard:\n%d %d %d %d %d %d\n%d %d %d %d %d %d\n",
                         p->pseudo_joueur1, p->score_joueur1, p->pseudo_joueur2, p->score_joueur2,
                         p->cases[0], p->cases[1], p->cases[2], p->cases[3], p->cases[4], p->cases[5],
                         p->cases[11], p->cases[10], p->cases[9], p->cases[8], p->cases[7], p->cases[6]);

                send_to_client(clients[sender_index].sock, board_msg);
                send_to_client(clients[receiver_index].sock, board_msg);

                printf("Game session %d initialized and board sent to players.\n", game_index);
            } else {
                send_to_client(clients[sender_index].sock, "Failed to create game session.\n");
                send_to_client(clients[receiver_index].sock, "Failed to create game session.\n");
            }

            pending_invites[i].sender_id = -1;
            pending_invites[i].receiver_id = -1;
            return;
        }
    }

    send_to_client(clients[receiver_index].sock, "No pending invitation found.\n");
}

void print_pending_invites() {
    printf("Pending Invitations:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pending_invites[i].sender_id != -1 && pending_invites[i].receiver_id != -1) {
            printf("  Sender: %d, Receiver: %d\n", 
                pending_invites[i].sender_id, pending_invites[i].receiver_id);
        }
    }
}

void handle_game_move(int game_index, int sender_id, int move) {
    if (game_index < 0 || game_index >= MAX_CLIENTS / 2 || games[game_index].client1_id == -1) {
        printf("Invalid game session or index: %d\n", game_index);
        return;
    }

    GameSession *game = &games[game_index];
    Plateau *p = &game->plateau;

    // Find sender index
    int sender_index = find_client_by_id(sender_id);
    if (sender_index == -1) {
        printf("Error: Sender with ID %d not found.\n", sender_id);
        return;
    }

    // Validate the sender
    if ((p->joueur_courant == 0 && sender_id != game->client1_id) ||
        (p->joueur_courant == 1 && sender_id != game->client2_id)) {
        printf("Invalid move: Sender %d is not the current player.\n", sender_id);
        return;
    }

    // Process the move
    if (!jouer_coup(p, move)) {
        send_to_client(clients[sender_index].sock, "Invalid move. Try again.\n");
        return;
    }

    // Notify players about the board state
    char board_msg[BUF_SIZE];
    snprintf(board_msg, BUF_SIZE, "Board updated:\nPlayer 1 (%s): %d\nPlayer 2 (%s): %d\nBoard:\n%d %d %d %d %d %d\n%d %d %d %d %d %d\n",
        p->pseudo_joueur1, p->score_joueur1,
        p->pseudo_joueur2, p->score_joueur2,
        p->cases[0], p->cases[1], p->cases[2], p->cases[3], p->cases[4], p->cases[5],
        p->cases[11], p->cases[10], p->cases[9], p->cases[8], p->cases[7], p->cases[6]);

    // Find other player index
    int other_index = (sender_id == game->client1_id)
        ? find_client_by_id(game->client2_id)
        : find_client_by_id(game->client1_id);

    if (other_index != -1) {
        send_to_client(clients[sender_index].sock, board_msg);
        send_to_client(clients[other_index].sock, board_msg);
    }

    // Check for victory
    if (verifier_victoire(p)) {
        char victory_msg[BUF_SIZE];
        snprintf(victory_msg, BUF_SIZE, "Game over! Winner: %s\n",
            p->score_joueur1 >= 25 ? p->pseudo_joueur1 : p->pseudo_joueur2);
        send_to_client(clients[sender_index].sock, victory_msg);
        send_to_client(clients[other_index].sock, victory_msg);

        // Reset the game session
        if (sender_index != -1) clients[sender_index].active = 0;
        if (other_index != -1) clients[other_index].active = 0;
        game->client1_id = -1;
        game->client2_id = -1;
    }
}

int find_client_by_id(int client_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].id == client_id) {
            return i; // Found the client
        }
    }
    return -1; // Client not found
}

void handle_decline_command(int receiver_index) {
    int receiver_id = clients[receiver_index].id;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pending_invites[i].receiver_id == receiver_id) {
            int sender_id = pending_invites[i].sender_id;

            // Map sender_id to its client index
            int sender_index = -1;
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].id == sender_id) {
                    sender_index = j;
                    break;
                }
            }

            // Validate sender_index
            if (sender_index == -1 || clients[sender_index].sock < 0) {
                printf("Error: Sender not found or socket invalid.\n");
                send_to_client(clients[receiver_index].sock, "Error: Sender not found.\n");
                return;
            }

            // Notify the sender and receiver
            send_to_client(clients[sender_index].sock, "Your invitation was declined.\n");
            send_to_client(clients[receiver_index].sock, "You declined the invitation.\n");

            // Clear the pending invite
            pending_invites[i].sender_id = -1;
            pending_invites[i].receiver_id = -1;
            return;
        }
    }

    send_to_client(clients[receiver_index].sock, "No pending invitation found.\n");
}

int find_game_by_client_id(int client_id) {
    for (int i = 0; i < MAX_CLIENTS / 2; i++) {
        if (games[i].client1_id == client_id || games[i].client2_id == client_id) {
            return i; // Found the game
        }
    }
    return -1; // No game found
}
