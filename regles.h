#ifndef REGLES_H
#define REGLES_H

#define MAX_PSEUDO_LENGTH 50

typedef struct {
    int cases[12];          // Game board (number of seeds in each pit)
    int joueur_courant;     // 0 for player 1, 1 for player 2
    int score_joueur1;      // Player 1's score
    int score_joueur2;      // Player 2's score
    char pseudo_joueur1[MAX_PSEUDO_LENGTH]; // Player 1's pseudonym
    char pseudo_joueur2[MAX_PSEUDO_LENGTH]; // Player 2's pseudonym
} Plateau;

// Initialize the game board
void initialiser_plateau(Plateau* p);

// Display the game board (for debugging or server logs)
void afficher_plateau(const Plateau* p);

// Play a move and update the game state
int jouer_coup(Plateau* p, int case_depart);

// Check if a player has won
int verifier_victoire(const Plateau* p);

#endif // REGLES_H
