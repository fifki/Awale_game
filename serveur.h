#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>
#include "rules.h"

#define PORT 1975
#define MAX_CLIENTS 10
#define BUF_SIZE 1024






int init_server_socket();
void handle_new_client(int server_sock);
void handle_client_message(int client_index);
void broadcast_clients_list();
void send_to_client(int client_sock, const char *message);
void remove_client(int index);
int create_game_session(int client1_id, int client2_id);
void handle_game_move(int game_index, int sender_id, int move);
void handle_game_invitation(int sender_index, int target_id);
int find_game_by_client_id(int client_id);
void handle_accept_command(int receiver_index);
void handle_decline_command(int receiver_index);
void print_pending_invites();


int find_client_by_id(int client_id);
