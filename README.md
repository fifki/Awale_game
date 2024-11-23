# Awale_game
This project implements the game as a client-server application, where players connect to a central server to play.
  ## Features
Multiplayer support for up to 10 players.
Clients can:
-Choose a pseudonym upon connection.
-Invite other players to a game.
-Accept or decline invitations.
-Play the game in real-time.
-Chat with their opponent during the game.
Players can quit the game while staying connected to the server.
Server supports:
-Tracking active and inactive players.
-Managing multiple simultaneous games.
 ## Requirements
Operating Systems: Linux or Windows
Dependencies:
 - GCC (GNU Compiler Collection)
 - telnet (for testing the client)
 ## Installation
### Linux
Clone the repository:
 - git clone https://github.com/your-repo/awale-game.git
 - cd awale-game
Compile the server and client:
 - gcc -o serveur serveur.c regles.c
 - gcc -o client client2.c
Run :
 - ./server
 - ./client <server-ip> 

### Windows
Clone the repository using Git Bash or any Git client.
Compile the server and client with a compatible compiler (e.g., MinGW):
 - gcc -o server server2.c rules.c
 - gcc -o client client2.c
Run the server:
 - server.exe

###  Commands
Pseudonym Setup: Enter your pseudonym upon connecting.
Game Commands:
 - INVITE <playerId>: Invite another player to a game.
 - ACCEPT: Accept a game invitation.
 - DECLINE: Decline a game invitation.
 - MOVE <pitNumber>: Make a move by selecting a pit (1–6).
 - MESSAGE <text>: Send a message to your opponent.
 - QUIT: Quit the game but remain connected to the server.
 - EXIT: Disconnect from the server.



