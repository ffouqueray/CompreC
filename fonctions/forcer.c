#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>

#include "navigation.h"

// Code qui sert a l'ouverture avec un mot de passe
struct zip* open_zip_with_password(const char *archive, const char *password) {
    int err;
    struct zip* za;

    // On essaie d'ouvrir le fichier d'archive
    if ((za = zip_open(archive, 0, &err)) == NULL) {
        printf("Unable to open archive %s\n", archive);
        return NULL;
    }

    // On recupere tous les fichiers presents dans l'archive et on essaie de les dechiffrer avec le mdp qu'on a donne
    int num_files = zip_get_num_entries(za, 0);
    if (verbose == 1) {
        printf("Trying password '%s'\n", guess);
    }
    for(int i = 0; i < num_files; i++) {
        const char *name = zip_get_name(za, i, 0);

        if (name == NULL) {
            printf("Can't retrieve the name of the file at index %d in the archive\n", i);
            zip_close(za);
            return NULL;
        }

        // On tente de l'ouvrir avec le mot de passe
        struct zip_file *zf = zip_fopen_encrypted(za, name, 0, password);
        if (zf == NULL) {
            zip_close(za);
            return NULL;
        }

        zip_fclose(zf);
    }

    // On met le mot de passe par defaut de l'archive si ca a reussi a dechiffrer avec ce mot de passe
    if (zip_set_default_password(za, password) != 0) {
        printf("Failed to set default password\n");
        zip_close(za);
        return NULL;
    }

    // On sauvegarde le mot de passe dans une variable globale
    global_pwd = strdup(password);

    return za;
}

// Code qui sert a faire notre attaque de brute force sur X caracteres, et renvoie le mdp une fois trouve. Sinon elle renvoie NULL
char* brute_force_attack(const char *archive, int max_length) {
    struct zip *za;
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int charset_len = strlen(charset);
    
    // Une boucle pour les caractères de 1 a max defini
    for (int length = 1; length <= max_length; length++) {
        char *guess = (char *)malloc((length + 1) * sizeof(char));
        memset(guess, charset[0], length);
        guess[length] = '\0';

        do {
            za = open_zip_with_password(archive, guess);
            if (za != NULL) {
                return guess;  // Password is correct, return it.
            }

            // Generate next password
            int index = length - 1;
            while (index >= 0) {
                char* pos = strchr(charset, guess[index]);
                if (pos < charset + charset_len - 1) {  // If not at the last char in charset
                    guess[index] = *(pos + 1);  // Move to the next char in charset
                    break;
                } else {
                    guess[index] = charset[0];  // Reset this index to the first char in charset
                    index--;
                }
            }
        } while (guess[0] != charset[charset_len - 1] || guess[length - 1] != charset[charset_len - 1]);

        free(guess);
    }

    return NULL;  // No password was found within the max_length
}
