#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <pthread.h>
#include <math.h>

#include "navigation.h"

// On fait des variables globales au fichier pour nos mots de passe
pthread_mutex_t password_mutex = PTHREAD_MUTEX_INITIALIZER;
char *current_password = NULL;
char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
int charset_len;
int current_password_length = 0;
int current_password_index = 0;

// On fait notre propre structure de donnee pour les threads
typedef struct {
    const char *archive;
    char *charset;
} thread_data_t;

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
        printf("Trying password '%s'\n", password);
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

// Fonction dediee à la generation de nouveaux mots de passe
char *next_password() {

    // On verrouille le mutex pour éviter que plusieurs threads ne modifient le mot de passe courant en même temps
    pthread_mutex_lock(&password_mutex);
    
    // Si c'est le premier mot de passe a generer
    if (current_password == NULL) {
        // On alloue la memoire et initialisons toutes les variables
        current_password = malloc(2);
        current_password[0] = charset[0];
        current_password[1] = '\0';
        current_password_length = 1;
        current_password_index = 0;
    } else {
        // On incremente l'index
        current_password_index++;

        // Si on a fait le tour de la liste sur le nombre de combinaisons de cette longueur
        if (current_password_index >= pow(charset_len, current_password_length)) {
            // On alloue la memoire et reinitialisons l'index
            current_password_length++;
            current_password_index = 0;
            current_password = realloc(current_password, current_password_length + 1);
        }

        // On génère le nouveau mot de passe en utilisant l'index du mot de passe et l'ensemble de caractères
        for (int i = 0; i < current_password_length; i++) {
            int index = (current_password_index / (int)pow(charset_len, i)) % charset_len;
            current_password[i] = charset[index];
        }

        // On ajoute le caractère nul de fin de chaîne
        current_password[current_password_length] = '\0';
    }

    // On cree une copie et on deverrouille le mutex
    char *password = strdup(current_password);
    pthread_mutex_unlock(&password_mutex);

    return password;
}

// Fonction executee par chaque thread
void *brute_force_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    char *password;

    while ((password = next_password()) != NULL) {
        open_zip_with_password(data->archive, password);

        free(password);

        // On check si un autre thread a trouve le mot de passe
        if (global_pwd != NULL) {
            pthread_exit(NULL);
        }
    }

    return NULL;
}

// Fonction principale pour lancer l'attaque de brute force
char* brute_force_attack(const char *archive, int num_threads) {
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];

    charset_len = strlen(charset);

    for (int i = 0; i < num_threads; i++) {
        thread_data[i].archive = archive;
        pthread_create(&threads[i], NULL, brute_force_thread, &thread_data[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);

        // On check si un thread a trouve le mot de passe
        if (global_pwd != NULL) {
            // On annule tous les autres threads
            for(int j = 0; j < num_threads; j++) {
                if(i != j) {
                    pthread_cancel(threads[j]);
                }
            }

            free(current_password);
            return global_pwd;
        }
    }

    free(current_password);
    return NULL;
}
