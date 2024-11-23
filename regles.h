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


void initialiser_plateau(Plateau* p);

void afficher_plateau(const Plateau* p);

int jouer_coup(Plateau* p, int case_depart);

int verifier_victoire(const Plateau* p);

#endif 
