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
    for (int i = 0; i < 12; i++) {
        p->cases[i] = 4;  // Chaque case commence avec 4 graines
    }
    /* ==== CAS DE FAMINE SI LE JOUEUR1 CHOISI LA CASE 6 ======
    p->cases[0] = 2;
    p->cases[1] = 7;
    p->cases[2] = 0;
    p->cases[3] = 0;
    p->cases[4] = 0;
    p->cases[5] = 1;
    p->cases[6] = 2;
    p->cases[7] = 0;
    p->cases[8] = 0;
    p->cases[9] = 0;
    p->cases[10] = 0;
    p->cases[11] = 0;
    */

    /* ==== CAS DE FAMINE SI LE JOUEUR1 CHOISI LA CASE 6 ======*/
    p->cases[0] = 2;
    p->cases[1] = 14;
    p->cases[2] = 0;
    p->cases[3] = 0;
    p->cases[4] = 0;
    p->cases[5] = 1;
    p->cases[6] = 2;
    p->cases[7] = 0;
    p->cases[8] = 0;
    p->cases[9] = 0;
    p->cases[10] = 0;
    p->cases[11] = 0;
    


    p->score_joueur1 = 0;
    p->score_joueur2 = 0;
    p->joueur_courant = 0;  // Joueur 1 commence
}

// Fonction pour afficher l'état du plateau
void afficher_plateau(const Plateau* p) {
    printf("\n %s (Joueur 1) : %d | %s (Joueur 2) : %d\n", p->pseudo_joueur1, p->score_joueur1, p->pseudo_joueur2, p->score_joueur2);
    
    for (int i = 11; i >= 6; i--) {
        printf("%d ", p->cases[i]);
    }
    printf("\n");
    for (int i = 0; i < 6; i++) {
        printf("%d ", p->cases[i]);
    }
    printf("\n");
    printf("A %s de jouer\n", p->joueur_courant == 0 ? p->pseudo_joueur1 : p->pseudo_joueur2);
    printf("Tapez '0' pour abandonner la partie.\n");
}

// Fonction pour jouer un coup
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

    // Vérification si la case contient 12 graines ou plus
    int sauter_case = (graines_a_distribuer >= 12);  // Si la case choisie contient 12 graines ou plus, on doit la sauter

    // Distribution des graines
    while (graines_a_distribuer > 0) {
        index = (index + 1) % 12;  // On passe à la case suivante en boucle

        // Si on doit sauter la case d'origine (si elle avait 12 graines ou plus)
        if (sauter_case && index == case_depart) {
            continue;  // On ne remet pas de graines dans la case d'origine
        }

        p->cases[index]++;
        graines_a_distribuer--;
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

// Fonction principale pour gérer le jeu
int main() {
    Plateau plateau;
    
    // Demander aux joueurs de choisir un pseudo
    printf("Joueur 1, entrez votre pseudo : ");
    fgets(plateau.pseudo_joueur1, MAX_PSEUDO_LENGTH, stdin);
    plateau.pseudo_joueur1[strcspn(plateau.pseudo_joueur1, "\n")] = '\0';  // Enlever le \n de fgets

    printf("Joueur 2, entrez votre pseudo : ");
    fgets(plateau.pseudo_joueur2, MAX_PSEUDO_LENGTH, stdin);
    plateau.pseudo_joueur2[strcspn(plateau.pseudo_joueur2, "\n")] = '\0';  // Enlever le \n de fgets

    initialiser_plateau(&plateau);

    while (!verifier_victoire(&plateau)) {
        afficher_plateau(&plateau);
        printf("%s, choisissez une case (1-6 pour votre côté) : ", plateau.joueur_courant == 0 ? plateau.pseudo_joueur1 : plateau.pseudo_joueur2);
        int case_depart;
        scanf("%d", &case_depart);

        if (case_depart == 0) {
            // Le joueur abandonne et l'autre gagne
            printf("%s a abandonné. %s gagne la partie !\n", plateau.joueur_courant == 0 ? plateau.pseudo_joueur1 : plateau.pseudo_joueur2, plateau.joueur_courant == 0 ? plateau.pseudo_joueur2 : plateau.pseudo_joueur1);
            break;
        }

        if (case_depart < 1 || case_depart > 6) {
            printf("Entrée invalide. Veuillez choisir un numéro entre 1 et 6.\n");
            continue;
        }

        // Ajuster case_depart pour correspondre aux indices du tableau
        if (plateau.joueur_courant == 0) {
            case_depart -= 1;  // Joueur 1 : cases 1-6 deviennent indices 0-5
        } else {
            case_depart += 5;  // Joueur 2 : cases 1-6 deviennent indices 6-11
        }

        if (!jouer_coup(&plateau, case_depart)) {
            printf("Coup invalide. Réessayez.\n");
        }
    }

    return 0;
}
