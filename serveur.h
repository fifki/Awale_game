#ifndef SERVER_H
#define SERVER_H


#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib") 
typedef int socklen_t;
#define close(s) closesocket(s)

#elif defined(linux)
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> 
#include <netdb.h>  
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else
#error Platform not supported
#endif


#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define BUF_SIZE    1024

#include "rules.h"
#include "client2.h"

// prototypes
int init_server_socket();
void handle_new_client(int server_sock);
void handle_client_message(int client_index);
void handle_quit_command(int client_index);
void broadcast_clients_list();
void send_to_client(SOCKET client_sock, const char *message);
void remove_client(int client_index);
int find_game_by_client_id(int client_id);
int create_game_session(int client1_id, int client2_id);
void handle_game_invitation(int sender_index, int target_id);
void handle_accept_command(int receiver_index);
void handle_decline_command(int receiver_index);
void handle_game_move(int game_index, int sender_id, int move);
void print_pending_invites();
int find_client_by_id(int client_id);
int id_exist_already(int id);
void handle_game_message(int client_index, const char *message);

#endif 
