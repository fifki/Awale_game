#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Taille maximale des pseudos
#define MAX_PSEUDO_LENGTH 50

// Structure représentant le plateau de jeu
typedef struct {
    int cases[12];          // Tableau des 12 cases, avec le nombre de graines dans chaque case
    int joueur_courant;     // 0 pour joueur 1, 1 pour joueur 2
    int score_joueur1;      // Score du joueur 1
    int score_joueur2;      // Score du joueur 2
    char pseudo_joueur1[MAX_PSEUDO_LENGTH]; // Pseudo du joueur 1
    char pseudo_joueur2[MAX_PSEUDO_LENGTH]; // Pseudo du joueur 2
} Plateau;

// Fonction pour initialiser un plateau
void initialiser_plateau(Plateau* p) {
    if (p == NULL) {
    printf("Error: Null pointer passed to initialiser_plateau.\n");
    return;
    }
    for (int i = 0; i < 12; i++) {
        p->cases[i] = 4;  // Chaque case commence avec 4 graines
    }
   

    p->score_joueur1 = 0;
    p->score_joueur2 = 0;
    p->joueur_courant = 0;  // Joueur 1 commence
    strcpy(p->pseudo_joueur1, "");
    strcpy(p->pseudo_joueur2, "");
    printf("Initializing game board for session \n");

}

// Fonction pour afficher l'état du plateau
void afficher_plateau(const Plateau* p) {
    printf("%s (Joueur 1) : %d | %s (Joueur 2) : %d\n", p->pseudo_joueur1, p->score_joueur1, p->pseudo_joueur2, p->score_joueur2);
    for (int i = 0; i < 6; i++) {
        printf("%d ", p->cases[i]);
    }
    printf("\n");
    for (int i = 11; i >= 6; i--) {
        printf("%d ", p->cases[i]);
    }
    printf("\n");
    printf("A %s de jouer\n", p->joueur_courant == 0 ? p->pseudo_joueur1 : p->pseudo_joueur2);
    printf("Tapez '0' pour abandonner votre tour.\n");
}

// Fonction pour jouer un coup
int jouer_coup(Plateau* p, int case_depart) {
    int debut = p->joueur_courant == 0 ? 0 : 6;
    int fin = debut + 6;

    // Vérifier si la case appartient au joueur et n'est pas vide
    if (case_depart < debut || case_depart >= fin || p->cases[case_depart] == 0) {
        printf("Coup invalide. Choisissez une case appartenant à votre côté qui n'est pas vide.\n");
        return 0;  // Coup invalide
    }

    // Sauvegarder l'état du plateau avant de jouer
    Plateau plateau_sauvegarde = *p;

    int graines_a_distribuer = p->cases[case_depart];
    p->cases[case_depart] = 0;  // On enlève toutes les graines de la case choisie
    int index = case_depart;

    // Distribution des graines
    while (graines_a_distribuer > 0) {
        index = (index + 1) % 12;  // On passe à la case suivante en boucle
        if (index != case_depart) { // Ne remet pas les graines dans la case d'origine
            p->cases[index]++;
            graines_a_distribuer--;
        }
    }

    // Vérifier la famine avant de manger des graines
    int debut_adversaire = (p->joueur_courant == 0) ? 6 : 0;
    int fin_adversaire = debut_adversaire + 6;
    int famine = 1;  // On suppose qu'il y a une famine
    for (int i = debut_adversaire; i < fin_adversaire; i++) {
        if (p->cases[i] > 0) {
            famine = 0;  // Si l'adversaire a au moins une graine, pas de famine
            break;
        }
    }

    if (famine) {
        printf("Coup invalide car il entraîne une famine pour l'adversaire. Réessayez.\n");
        *p = plateau_sauvegarde;  // Rétablir l'état du plateau
        return 0;  // Coup invalide
    }

    // Capture des graines
    debut = p->joueur_courant == 0 ? 6 : 0;
    fin = p->joueur_courant == 0 ? 12 : 6;

    while (index >= debut && index < fin && (p->cases[index] == 2 || p->cases[index] == 3)) {
        if (p->joueur_courant == 0) {
            p->score_joueur1 += p->cases[index];
        } else {
            p->score_joueur2 += p->cases[index];
        }
        p->cases[index] = 0;
        index--;
    }

    // Vérifier la famine après avoir mangé les graines
    famine = 1;  // Réinitialiser la famine après avoir capturé les graines
    for (int i = debut_adversaire; i < fin_adversaire; i++) {
        if (p->cases[i] > 0) {
            famine = 0;  // Si l'adversaire a au moins une graine, pas de famine
            break;
        }
    }

    if (famine) {
        printf("Coup invalide car il entraîne une famine pour l'adversaire après avoir mangé les graines. Réessayez.\n");
        *p = plateau_sauvegarde;  // Rétablir l'état du plateau
        return 0;  // Coup invalide
    }

    // Changement de joueur
    p->joueur_courant = 1 - p->joueur_courant;
    return 1;  // Coup valide
}

// Fonction pour verifier la victoire
int verifier_victoire(const Plateau* p) {
    if (p->score_joueur1 >= 25) {
        printf("%s a gagné avec %d graines !\n", p->pseudo_joueur1, p->score_joueur1);
        return 1;
    } else if (p->score_joueur2 >= 25) {
        printf("%s a gagné avec %d graines !\n", p->pseudo_joueur2, p->score_joueur2);
        return 1;
    }
    
    return 0;
}


