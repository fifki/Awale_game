#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "server.h"
#include "rules.h"


typedef struct {
    int sock;
    char pseudoname[MAX_PSEUDO_LENGTH];
    int id;
    int active; 
} Client;

typedef struct {
    int client1_id;
    int client2_id;
    Plateau plateau; 
} GameSession;

typedef struct {
    int sender_id;
    int receiver_id;
} PendingInvite;


Client clients[MAX_CLIENTS];
GameSession games[MAX_CLIENTS / 2]; 
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

//assigning a new unique id to the client and register thier pseudoname 
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

            // Generate a unique ID
            int new_id;
            do {
                new_id = rand() % 10000; // Generate random ID
            } while (id_exist_already(new_id));

            clients[i].id = new_id;
            clients[i].active = 0; // Not in a game yet

            printf("New client connected: socket=%d, id=%d\n", new_sock, clients[i].id);
            send_to_client(new_sock, "Welcome! Enter your pseudoname: ");
            return; // Exit after assigning the client
        }
    }
        

    printf("Max clients reached. Connection refused.\n");
    close(new_sock);
}

//handing the inputs of my clients:
void handle_client_message(int client_index) {
    char buffer[BUF_SIZE];
    int n = recv(clients[client_index].sock, buffer, BUF_SIZE - 1, 0);

    if (n <= 0) {
        printf("Client %d disconnected.\n", clients[client_index].id);
        remove_client(client_index);
        return;
    }

    buffer[n] = '\0';
    printf("Message from %s: %s\n", 
        clients[client_index].pseudoname[0] == '\0' ? "Unknown" : clients[client_index].pseudoname, 
        buffer);

    // If the pseudoname is not set, treat the message as the pseudoname
    if (clients[client_index].pseudoname[0] == '\0') {
        strncpy(clients[client_index].pseudoname, buffer, MAX_PSEUDO_LENGTH - 1);
        clients[client_index].pseudoname[MAX_PSEUDO_LENGTH - 1] = '\0'; // Ensure null termination

        printf("Client %d set pseudoname: %s\n", clients[client_index].id, clients[client_index].pseudoname);

        // Send the manual after the pseudoname is set
        send_to_client(clients[client_index].sock, 
            "Welcome to the AWALE game :)\n"
            "To invite a player type : INVITE playerId\n"
            "  Example: INVITE 2556\n"
            "To accept an invitation type : ACCEPT\n"
            "To decline an invitation type : DECLINE\n"
            "During the game:\n"
            "  * Type MOVE number to play\n"
            "      Example: MOVE 5\n"
            "  * Type MESSAGE to communicate with the other player\n"
            "      Example : MESSAGE hi my name is fatima\n"
            "  * Type QUIT to quit the game\n"
            "  * Type EXIT to disconnect\n"
            "\n");

        broadcast_clients_list();
        return;
    }

    // Handle other client messages
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
    } else if (strncmp(buffer, "QUIT", 4) == 0) {
        handle_quit_command(client_index);
    } else if (strncmp(buffer, "EXIT", 4) == 0) {
        remove_client(client_index);
    } else if (strncmp(buffer, "MESSAGE", 7) == 0) {
        char *message = buffer + 8;
        handle_game_message(client_index, message);
    } else {
        send_to_client(clients[client_index].sock, "Unknown command.\n");
    }
}

//hand the invitation : not being able to invite one's self or a player
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
                    snprintf(invite_msg, BUF_SIZE, "INVITE from %s (ID: %d).(Type : ACCEPT or DECLINE )", 
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

void handle_decline_command(int receiver_index) {
    int receiver_id = clients[receiver_index].id;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (pending_invites[i].receiver_id == receiver_id) {
            int sender_id = pending_invites[i].sender_id;

            int sender_index = -1;
            for (int j = 0; j < MAX_CLIENTS; j++) {
                if (clients[j].id == sender_id) {
                    sender_index = j;
                    break;
                }
            }

            if (sender_index == -1 || clients[sender_index].sock < 0) {
                printf("Error: Sender not found or socket invalid.\n");
                send_to_client(clients[receiver_index].sock, "Error: Sender not found.\n");
                return;
            }

            send_to_client(clients[sender_index].sock, "Your invitation was declined.\n");
            send_to_client(clients[receiver_index].sock, "You declined the invitation.\n");

            pending_invites[i].sender_id = -1;
            pending_invites[i].receiver_id = -1;
            return;
        }
    }

    send_to_client(clients[receiver_index].sock, "No pending invitation found.\n");
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

            int sender_index = find_client_by_id(sender_id);
            if (sender_index == -1) {
                printf("Sender with ID %d not found.\n", sender_id);
                return;
            }

            // Create the session
            int game_index = create_game_session(sender_id, receiver_id);
            if (game_index != -1) {

                clients[sender_index].active = 1;
                printf("Debug: Client %d marked as active.\n", sender_id);

                clients[receiver_index].active = 1;
                printf("Debug: Client %d marked as active.\n", receiver_id);

                char msg[BUF_SIZE];
                snprintf(msg, BUF_SIZE, "Game accepted! You can now play.\n");
                send_to_client(clients[receiver_index].sock, msg);
                send_to_client(clients[sender_index].sock, msg);

                Plateau *p = &games[game_index].plateau;
                char initial_board[BUF_SIZE];

                

                if ( p -> joueur_courant == 0 ) {
                    snprintf (initial_board, BUF_SIZE,
                    "Game starts! It's the turn of %s . Initial board: \n Player 1 ( %s ): %d \n Player 2 ( %s ): %d \n Board: \n %d %d %d %d %d %d \n %d %d %d %d %d %d \n " ,
                    p -> pseudo_joueur1 , p -> pseudo_joueur1 , p -> score_joueur1 , p -> pseudo_joueur2 , p -> score_joueur2 ,
                    p -> cases [ 0 ], p -> cases [ 1 ], p -> cases [ 2 ], p -> cases [ 3 ], p -> cases [ 4 ], p -> cases [ 5 ],
                    p -> cases [ 11 ], p -> cases [ 10 ], p -> cases [ 9 ], p -> cases [ 8 ], p -> cases [ 7 ], p -> cases [ 6 ]); }
                    else if ( p -> joueur_courant == 1 ) {
                    snprintf (initial_board, BUF_SIZE,
                    "Game starts! It's the turn of %s . Initial board: \n Player 1 ( %s ): %d \n Player 2 ( %s ): %d \n Board: \n %d %d %d %d %d %d \n %d %d %d %d %d %d \n " ,
                    p -> pseudo_joueur2 , p -> pseudo_joueur1 , p -> score_joueur1 , p -> pseudo_joueur2 , p -> score_joueur2 ,
                    p -> cases [ 0 ], p -> cases [ 1 ], p -> cases [ 2 ], p -> cases [ 3 ], p -> cases [ 4 ], p -> cases [ 5 ],
                    p -> cases [ 11 ], p -> cases [ 10 ], p -> cases [ 9 ], p -> cases [ 8 ], p -> cases [ 7 ], p -> cases [ 6 ]);
                }

                send_to_client(clients[receiver_index].sock, initial_board);
                send_to_client(clients[sender_index].sock, initial_board);

                printf("Game session %d initialized and board sent to players.\n", game_index);
            } else {
                send_to_client(clients[receiver_index].sock, "Failed to create game session.\n");
                send_to_client(clients[sender_index].sock, "Failed to create game session.\n");
            }

            pending_invites[i].sender_id = -1;
            pending_invites[i].receiver_id = -1;
            return;
        }
    }

    send_to_client(clients[receiver_index].sock, "No pending invitation found.\n");
}
//intialise the game and the board 
int create_game_session(int client1_id, int client2_id) {
    int client1_index = find_client_by_id(client1_id);
    int client2_index = find_client_by_id(client2_id);

    if (client1_index == -1 || client2_index == -1) {
        printf("Invalid client IDs: %d or %d\n", client1_id, client2_id);
        return -1;
    }

    for (int i = 0; i < MAX_CLIENTS / 2; i++) {
        if (games[i].client1_id == -1) {
            games[i].client1_id = client1_id;
            games[i].client2_id = client2_id;

            initialiser_plateau(&games[i].plateau);
            strcpy(games[i].plateau.pseudo_joueur1, clients[client1_index].pseudoname);
            strcpy(games[i].plateau.pseudo_joueur2, clients[client2_index].pseudoname);

            // Mark both clients as active
            clients[client1_index].active = 1;
            clients[client2_index].active = 1;

            printf("Game session created between %d and %d\n", client1_id, client2_id);
            return i;
        }
    }

    return -1; // No available game slots
}

void handle_game_move(int game_index, int sender_id, int move) {
    if (game_index < 0 || game_index >= MAX_CLIENTS / 2 || games[game_index].client1_id == -1) {
        printf("Invalid game session or index: %d\n", game_index);
        return;
    }

    GameSession *game = &games[game_index];
    Plateau *p = &game->plateau;

    int sender_index = find_client_by_id(sender_id);
    if (sender_index == -1) {
        printf("Error: Sender with ID %d not found.\n", sender_id);
        return;
    }

    if ((p->joueur_courant == 0 && sender_id != game->client1_id) ||
        (p->joueur_courant == 1 && sender_id != game->client2_id)) {
        send_to_client(clients[sender_index].sock, "It's not your turn. Please wait.\n");
        return;
    }
    int test_case_invalide = 0 ;

    if (move < 1 || move > 6 ) {
    printf ( "Entrée invalide. \n " );
    test_case_invalide = 1 ;
    }

    if ( p -> joueur_courant == 0 ) {
    move -= 1 ; // Joueur 1 : cases 1-6 deviennent indices 0-5
    } else {
    move += 5 ; // Joueur 2 : cases 1-6 deviennent indices 6-11
    }

    if (test_case_invalide) {
    send_to_client ( clients [sender_index]. sock , "Invalid move. Try again.Please choose a number between 1 and 6 \n " );
    return ;
    }
    if (!jouer_coup(p, move)) {
        send_to_client(clients[sender_index].sock, "Invalid move. Try again.Please choose a number between 1 and 6\n");
        return;
    }

    char board_msg[BUF_SIZE];
    snprintf(board_msg, BUF_SIZE, "Board updated:\n%d %d %d %d %d %d\n%d %d %d %d %d %d\n",
        p->cases[0], p->cases[1], p->cases[2], p->cases[3], p->cases[4], p->cases[5],
        p->cases[11], p->cases[10], p->cases[9], p->cases[8], p->cases[7], p->cases[6]);

    send_to_client(clients[find_client_by_id(game->client1_id)].sock, board_msg);
    send_to_client(clients[find_client_by_id(game->client2_id)].sock, board_msg);

    // Notify players whose turn it is
    if (p->joueur_courant == 0) {
        send_to_client(clients[find_client_by_id(game->client1_id)].sock, "It's your turn.\n");
        send_to_client(clients[find_client_by_id(game->client2_id)].sock, "Waiting for the other player...\n");
    } else {
        send_to_client(clients[find_client_by_id(game->client2_id)].sock, "It's your turn.\n");
        send_to_client(clients[find_client_by_id(game->client1_id)].sock, "Waiting for the other player...\n");
    }

    if (verifier_victoire(p)) {
        char victory_msg[BUF_SIZE];
        snprintf(victory_msg, BUF_SIZE, "Game over! Winner: %s\n",
            p->score_joueur1 >= 25 ? p->pseudo_joueur1 : p->pseudo_joueur2);

        send_to_client(clients[find_client_by_id(game->client1_id)].sock, victory_msg);
        send_to_client(clients[find_client_by_id(game->client2_id)].sock, victory_msg);

        clients[find_client_by_id(game->client1_id)].active = 0;
        clients[find_client_by_id(game->client2_id)].active = 0;

        game->client1_id = -1;
        game->client2_id = -1;
    }
}

void handle_quit_command(int client_index) {
    if (client_index < 0 || client_index >= MAX_CLIENTS || clients[client_index].sock < 0) {
        printf("Error: Invalid client index.\n");
        return;
    }

    int client_id = clients[client_index].id;

    // Find the game session the client is in
    int game_index = find_game_by_client_id(client_id);
    if (game_index != -1) {
        GameSession *game = &games[game_index];
        int other_client_id = (game->client1_id == client_id) ? game->client2_id : game->client1_id;

        // Notify the other player in the game
        int other_index = find_client_by_id(other_client_id);
        if (other_index != -1 && clients[other_index].sock > 0) {
            send_to_client(clients[other_index].sock, "The other player has quit the game. You win by default.\n");
            clients[other_index].active = 0; // Reset the active status of the other player
        }

        // Reset the game session
        printf("Client %d quit the game. Game session %d ended.\n", client_id, game_index);
        game->client1_id = -1;
        game->client2_id = -1;
    }

    // Mark the client as inactive
    clients[client_index].active = 0;
    send_to_client(clients[client_index].sock, "You have quit the game but remain connected to the server.\n");
    printf("Client %d left the game but remains connected.\n", client_id);

    // Broadcast updated client list to others
    broadcast_clients_list();
}
 
 //to be able to communicate with the other player
void handle_game_message(int client_index, const char *message) {
    int client_id = clients[client_index].id;

    // Find the game session the client is in
    int game_index = find_game_by_client_id(client_id);
    if (game_index == -1) {
        send_to_client(clients[client_index].sock, "You are not in a game.\n");
        return;
    }

    // Determine the other client in the game
    GameSession *game = &games[game_index];
    int other_client_id = (game->client1_id == client_id) ? game->client2_id : game->client1_id;
    int other_index = find_client_by_id(other_client_id);

    if (other_index == -1 || clients[other_index].sock <= 0) {
        send_to_client(clients[client_index].sock, "The other player is unavailable.\n");
        return;
    }

    // Forward the message to the other client
    char msg_to_send[BUF_SIZE];
    snprintf(msg_to_send, BUF_SIZE, "Message from %s: %s\n", clients[client_index].pseudoname, message);
    send_to_client(clients[other_index].sock, msg_to_send);

    printf("Message from client %d forwarded to client %d.\n", client_id, other_client_id);
}

//send messages to all the client or only non active ones 
void broadcast_clients_list() {
    char list[BUF_SIZE] = "Connected clients:\n";

    // Build the list of clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0) { // all the clients 
            char entry[128];
            snprintf(entry, sizeof(entry), "ID: %d, Name: %s\n", clients[i].id, clients[i].pseudoname);
            strncat(list, entry, sizeof(list) - strlen(list) - 1);
        }
    }

    //  only non-active clients
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if ((clients[i].sock > 0) && (!clients[i].active)) { 
            send_to_client(clients[i].sock, list);
        }
    }
}

 //send to a chosen client
void send_to_client(int client_sock, const char* message) {
    if (client_sock < 0) {
        printf("Error: Invalid socket descriptor.\n");
        return;
    }
    if (send(client_sock, message, strlen(message), 0) < 0) {
        perror("send() failed");
    }
}


int find_client_by_id(int client_id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].id == client_id) {
            return i; 
        }
    }
    return -1; 
}
int find_game_by_client_id(int client_id) {

    for (int i = 0; i < MAX_CLIENTS / 2; i++) {
        if (games[i].client1_id == client_id || games[i].client2_id == client_id) {
            return i; // Found the game
        }
    }
    return -1; // No game found
}
int id_exist_already(int id){
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].id == id) {
            return 1; 
        }
    }
    return 0; 
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

//removge the client and notify the other player
void remove_client(int client_index) {
    if (client_index < 0 || client_index >= MAX_CLIENTS || clients[client_index].sock < 0) {
        return;
    }

    int client_id = clients[client_index].id;

    // Find the game session the client is in
    int game_index = find_game_by_client_id(client_id);
    if (game_index != -1) {
        GameSession *game = &games[game_index];
        int other_client_id = (game->client1_id == client_id) ? game->client2_id : game->client1_id;

        // Notify the other player in the game
        int other_index = find_client_by_id(other_client_id);
        if (other_index != -1 && clients[other_index].sock > 0) {
            char winner_msg[BUF_SIZE];
            snprintf(winner_msg, BUF_SIZE, "The other player has disconnected. You win by default!\n");
            send_to_client(clients[other_index].sock, winner_msg);
            clients[other_index].active = 0; // Reset the active status of the other player
        }

        // Reset the game session
        printf("Client %d disconnected. Game session %d ended.\n", client_id, game_index);
        game->client1_id = -1;
        game->client2_id = -1;
    }

    // Close the client's socket and reset their state
    close(clients[client_index].sock);
    clients[client_index].sock = -1;
    clients[client_index].active = 0;
    clients[client_index].pseudoname[0] = '\0';
    printf("Client %d removed.\n", client_id);

    // Broadcast updated client list to others
    broadcast_clients_list();
}
