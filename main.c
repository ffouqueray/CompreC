#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <getopt.h>

#include <zip.h>

// declaration des macro preprocesseurs
#if defined(_WIN32) || defined(_WIN64)
    #include "fonctions\navigation.h"
    #include "fonctions\forcer.h"
#else
    #include "fonctions/navigation.h"
    #include "fonctions/forcer.h"
#endif

/*
        Commande à utiliser pour compiler le programme :
             gcc fonctions/*.c main.c -lzip -o main
*/

// Commande qui affiche le help message si jamais on lance le programme avec le -h ou seul
void help(char *argv[]) {
    printf(
        "Usage: %s [OPTION]... [FILE]...\nOptions:\n\t-h, --help\t\t\tShow this help\n\t-o, --open\t\t\tOpen a zip file for browsing (necessary)\n\t-b, --bruteforce\t\tTry to bruteforce the password\n\t-d, --dictionary FILE\t\tTry to bruteforce the password with a dictionary\n\t-p, --password PASSWORD\t\tUse this password\n\t-v, --verbose\t\t\tTo active verbose mode\nExamples :\n\t%s -o --password MyPassword my-archive.zip\n\t%s -o -b -d my-text-dictionary.txt my-archive.zip\n",
        argv[0],
        argv[0],
        argv[0]
    );
}

int main(int argc, char* argv[]) {
    struct zip* za;
    struct zip_stat sb;
    int err;
    char *archive = NULL, *dict_file = NULL, *pwd = NULL;
    int encrypted = 0, bruteforce = 0, done = 0;
    int opt;

    // Structure de notre getopt avec les formats courts et longs
    static struct option long_options[] = {
        {"open",        required_argument, 0, 'o'},
        {"bruteforce",  no_argument,       0, 'b'},
        {"dictionary",  required_argument, 0, 'd'},
        {"password",    required_argument, 0, 'p'},
        {"help",        no_argument,       0, 'h'},
        {"verbose",     no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    // Arguments de notre getopt
    while ((opt = getopt_long(argc, argv, "o:bd:vp:h", long_options, &long_index)) != -1) {
        switch (opt) {
            case 'o':
                archive = optarg;
                printf("%s", archive);
                break;
            case 'v':
                verbose = 1;
                break;
            case 'b':
                bruteforce = 1;
                break;
            case 'd':
                dict_file = optarg;
                break;
            case 'p':
                pwd = optarg;
                break;
            case 'h':
                help(argv);
                return 0;
            default:
                help(argv);
                return 0;
        }
    }

    // On check si on a entre un fichier d'ouverture du zip ou pas
    if (archive != NULL) {
        if ((za = zip_open(archive, 0, &err)) == NULL) {
            printf("Unable to open archive %s\n", archive); // Si on arrive pas a ouvrir l'archive parce qu'il a mis le -o mais pas le chemin du zip
            return 1;
        }
    } else {
        // ce cas n'est pas cense arriver mais il est possible que ca survienne avec des bugs (si c'est mal fix)
        help(argv);
        return 1;
    }

    // On fait une boucle qui fait une verification sur le zip pour savoir s'il est chiffre ou pas
    for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
        if (zip_stat_index(za, i, 0, &sb) == 0) {
            if (sb.encryption_method != ZIP_EM_NONE) {
                encrypted = 1;
                break; // s'il est chiffre on passe encrypted a 1 et on quitte la boucle
            }
        }
    }

    // On affiche si c'est chiffre ou pas
    if (encrypted) {
        printf("The archive is encrypted.\n");

        // Si on a passe un mot de passe en dur
        if (pwd != NULL) {
            printf("Trying to open with the provided password.\n");
            za = open_zip_with_password(archive, pwd);
            if (za == NULL) {
                printf("Could not open the zip using password '%s'.\n", pwd);
            } else {
                printf("The password '%s' is correct\nOpening the ZIP file...\n", global_pwd);
                navigate(za, (char *) archive);
                done = 1;
            }
        }

        // Si on a passe un fichier pour une attaque par dictionnaire
        if (dict_file != NULL && done == 0) {
            FILE *file = fopen(dict_file, "r");
            if (file == NULL) {
                printf("Could not open the dictionary file: %s\n", dict_file);
            } else {

                char line[256];
                while (fgets(line, sizeof(line), file)) {
                    // On retire le retour a la ligne de notre ligne dans le fichier
                    line[strcspn(line, "\n")] = 0;

                    za = open_zip_with_password(archive, line);
                    if (za != NULL) {
                        printf("The password '%s' is correct\nOpening the ZIP file...\n", line);
                        navigate(za, (char *) archive);
                        done = 1;
                        break;
                    }
                }
                fclose(file);
            }
        }

        // Si on a passe notre argument pour du bruteforce
        if (bruteforce  && done == 0) {
            char *mot_de_passe;
            mot_de_passe = brute_force_attack(archive, 15);  // Le 15 est la longueur max du mot de passe, on peut le readapter pour le passer en argument getopt
            if (mot_de_passe != NULL) {
                za = open_zip_with_password(archive, mot_de_passe);
                printf("The password '%s' is correct\nOpening the ZIP file...\n", mot_de_passe);
                free(mot_de_passe);
                navigate(za, (char *) archive);
                done = 1;
            } else {
                printf("Could not bruteforce the zip password using the maximum 15 characteres.\n");
            }
        }

    } else { // Si l'archive n'est pas chiffree
        printf("The archive is not encrypted.\nOpening the ZIP file...\n");
        navigate(za, (char *) archive);
    }

    zip_close(za);

    return 0;
}