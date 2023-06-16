#include <zip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

#include "colors.h"
#include "shell_cmd.h"
#include "shell_dependencies.h"

#define MAX_PATH_LENGTH 1024

extern int verbose;

// Code de la commande "help"
void cmd_help() {
    printf("Commands :\n\thelp\t\tshow this command\n\tcd\t\tto go in a directory / go out of a directory\n\tls\t\tto show the content of a directory\n\texport\t\tto export a file or a directory out of the zip\n\timport\t\tto import a file or a directory from the computer to the zip.\t\t\t\t\t(Unencrypted ZIPs only)\n\trm\t\tto remove a file or a directory from the zip (a rm command will close the archive after use).\t(Unencrypted ZIPs only)\nExamples :\n\tls\n\tTo show the content of the current directory\n\n\tcd test folder\n\tTo go to the folder named \"test folder\"\n\n\tcd ..\n\tTo go to the parent folder\n\n\texport filename in zip*destination path\n\tTo export the file named \"filename in zip\" in the path \"destination path\"\n\n\texport filename in zip.txt\n\tTo export the file named \"filename in zip.txt\" in the path of the executable\n\n\timport folder\n\tTo import all content of the directory named \"folder\" in you current path\n\n\trm folder\n\tTo remove all content of the directory named \"folder\" and the folder itself\n");
}


// Code de la commande "ls"
void cmd_ls(struct zip *za, const char *prefix) {
    int num_files = zip_get_num_entries(za, 0); // Recupere le nombre total d'entrees dans l'archive
    size_t prefix_len = strlen(prefix); // Recupere la longueur du préfixe (nom du dossier actuel via la commande "cd")
    char **printed_folders = NULL; // Initialisation du tableau qui va contenir les noms de dossiers deja affiches
    int printed_folders_count = 0; // Compteur du nombre de dossiers deja affiches

    for (int i = 0; i < num_files; i++) { // Pour chaque entree de l'archive
        struct zip_stat sb;
        zip_stat_index(za, i, 0, &sb); // Informations sur l'entree actuelle (le fichier actuel)

        // Verification si le nom de l'entree commence par le préfixe
        if (strncmp(sb.name, prefix, prefix_len) == 0) {
            // On recupere la partie du nom qui suit le préfixe
            const char *remainder = sb.name + prefix_len;
            char *slash = strchr(remainder, '/'); // On cherche un slash dans la partie restante du nom
            if (slash == NULL) {
                green(); // On passe le texte en vert
                // S'il n'y a pas de slash, alors c'est un fichier directement dans le dossier actuel et on l'affiche
                printf("%s\n", remainder);
            } else {
                // S'il y a un slash, alors c'est un fichier dans un sous-dossier donc on affiche le sous-dossier
                char *folder_name = (char *)malloc((slash - remainder + 2) * sizeof(char));
                strncpy(folder_name, remainder, slash - remainder + 1);
                folder_name[slash - remainder + 1] = '\0';
                
                int already_printed = 0;
                for (int j = 0; j < printed_folders_count; j++) {
                    // On verifie si le nom du dossier est deja dans notre tableau de dossiers affiches
                    if (strcmp(printed_folders[j], folder_name) == 0) {
                        already_printed = 1;
                        break;
                    }
                }
                if (already_printed != 1) {
                    blue(); // On passe le texte en bleu
                    // Si le nom du dossier n'a pas encore ete affiche, on l'ajoute au tableau et on l'affiche
                    printf("%s\n", folder_name);
                    printed_folders = realloc(printed_folders, sizeof(char*) * (printed_folders_count + 1)); // On agrandit notre tableau dynamique
                    printed_folders[printed_folders_count] = folder_name; // On ajoute le nom du dossier à notre tableau
                    printed_folders_count++; // On incremente notre compteur de dossiers affiches
                } else {
                    // Si le nom du dossier a déjà été imprimé, on libère simplement la mémoire allouée par strndup
                    free(folder_name);
                }
            }
        }
    }

    // Liberation de la memoire allouee pour le tableau et ses elements
    for (int i = 0; i < printed_folders_count; i++) {
        free(printed_folders[i]);
    }
    free(printed_folders);

    reset();
}

// Code de la commande "cd"
void cmd_cd(struct zip *za, const char *new_dir, char *prefix) {
    if (strcmp(new_dir, "..") == 0) {
        // Si nous sommes deja a la racine, on ne fait rien
        if (strlen(prefix) == 0) {
            return;
        }

        // Retirer le slash de fin s'il est present
        if (prefix[strlen(prefix) - 1] == '/') {
            prefix[strlen(prefix) - 1] = '\0';
        }

        // Trouver le dernier slash après avoir éventuellement retiré le slash de fin
        char *last_slash = strrchr(prefix, '/');

        // Si un slash a ete trouve, on le remplace par '\0' pour terminer la chaîne de caractères
        if (last_slash != NULL) {
            *(last_slash + 1) = '\0';
        } else {
            // Si aucun autre slash n'a ete trouve, on vide le prefixe
            prefix[0] = '\0';
        }
    } else if (strcmp(new_dir, "") == 0) {
        prefix[0] = '\0';

    } else {
        char new_prefix[256];

        // Vérifier si le dernier caractère de new_dir est un '/'
        if (new_dir[strlen(new_dir) - 1] == '/') {
            // S'il y a déjà un '/', ne pas ajouter de slash supplémentaire
            snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, new_dir);
        } else {
            // S'il n'y a pas de '/', ajouter un slash
            snprintf(new_prefix, sizeof(new_prefix), "%s%s/", prefix, new_dir);
        }

        if (dir_exists(za, new_dir, prefix)) {
            strcpy(prefix, new_prefix);
        } else {
            printf("The folder '%s' does not exist.\n", new_dir);
        }
    }
}

// Code de la commande "export"
void cmd_export_recursive(struct zip *za, const char *filename_in_zip, const char *destination_path, char *prefix) {
    struct zip_stat sb;
    zip_stat_init(&sb);
    zip_stat(za, filename_in_zip, 0, &sb);


    char dir_name_with_slash[256];
    // Copie le nom du repertoire dans dir_name_with_slash
    strncpy(dir_name_with_slash, filename_in_zip, sizeof(dir_name_with_slash) - 1);
    // S'assure que la chaine se termine par '\0'
    dir_name_with_slash[sizeof(dir_name_with_slash) - 1] = '\0';

    // Si le dernier caractere n'est pas un '/', alors on ajoute un '/'
    if (dir_name_with_slash[strlen(dir_name_with_slash) - 1] != '/') {
        strncat(dir_name_with_slash, "/", sizeof(dir_name_with_slash) - strlen(dir_name_with_slash) - 1);
    }

    // Copie le nom du repertoire dans destination_path_modified
    char destination_path_modified[256]; 
    strncpy(destination_path_modified, destination_path, sizeof(destination_path_modified));
    
    // Si le dernier caractere n'est pas un '\', alors on ajoute un '\' pour windows, sinon un /
    if (destination_path[strlen(destination_path) - 1] != file_separator[0]) {
        strncat(destination_path_modified, file_separator, sizeof(destination_path_modified) - strlen(destination_path_modified) - 1);
    }

    // Si un dossier existe sous ce nom, alors c'est un dossier a deplacer
    if (dir_exists(za, dir_name_with_slash, prefix)) {

        // On cree un path qui donnera la longueur en comptant le prefixe
        char new_path[256];
        snprintf(new_path, sizeof(new_path), "%s%s", prefix, dir_name_with_slash);

        // On recupere la liste des fichiers qui commencent par le chemin prefixe/recherche
        // et on stock les valeurs dans printed_items, et le nombre de valeurs dans printed_items_count
        int printed_items_count;
        const char **printed_items = list_files(za, new_path, &printed_items_count);

        // Pour chaque occurrence de printed_items, on remplace les / par le séparateur de l'OS
        // (utile pour windows)
        for (int i = 0; i < printed_items_count; i++) {
            printed_items[i] = replace_string(printed_items[i], "/", file_separator);
        }

        // On fait une copie de printed_items dans items_edition
        char **items_edition = malloc(sizeof(char*) * printed_items_count);
        for (int i = 0; i < printed_items_count; i++) {
            items_edition[i] = strdup(printed_items[i]);
        }

        // Pour chaque occurrence dans items_edition, on supprime tout ce qu'il y a après le dernier
        // file_separator (/ ou \) pour ne garder que les noms de dossier
        for (int i = 0; i < printed_items_count; i++) {
            // Trouver le dernier slash après avoir éventuellement retiré le slash de fin
            char *last_slash = strrchr(items_edition[i], file_separator[0]);

            // Si un slash a ete trouve, on remplace le caractre suivant par '\0'
            // pour terminer la chaîne de caractères
            if (last_slash != NULL) {
                *(last_slash + 1) = '\0';
            }
        }

        // On supprime le prefixe si on en a un, et cree un dossier pour chaque occurrence de items_edition
        // (verification d'existant dans la fonction create_dir)
        for (int i = 0; i < printed_items_count; i++) {
            const char *filename_export = remove_prefix(items_edition[i], prefix);
            create_dir(filename_export);
        }

        //char *last_sep = strrchr(destination_path, file_separator[0]);

        // Pour chaque occurrence recuperee, on verifie que ce soit pas un dossier
        // et si c'est pas le cas, on fait l'export du fichier
        for (int i = 0; i < printed_items_count; i++) {
            if (!(strcmp(items_edition[i], printed_items[i]) == 0)) {
                cmd_export(za, printed_items[i], destination_path_modified, prefix);
            }
        }

        // On vide la memoire allouee pour nos listes
        free(printed_items);
        for (int i = 0; i < printed_items_count; i++){
            free(items_edition[i]);
        }
        free(items_edition);
    } else {
        // Si dir_exists renvoie que c'est pas un dossier, alors on lance la manipulation pour
        // un fichier uniquement
        cmd_export(za, filename_in_zip, destination_path_modified, prefix);
    }
}


// Code de la commande "import"
void cmd_import_recursive(struct zip *za, char *filename, char *prefix) {  
    // On recupere le chemin absolu de notre chemin relatif, et faisons une copie qui nous servira  
    char *dossier_de_base = getAbsolutePath(filename);
    if (dossier_de_base == NULL) {
        return;
    }
    char *chemin_absolu = strdup(dossier_de_base);

    // On retire les \ et espaces de fin de notre chemin absolu
    removeTrailingFileSeparator(chemin_absolu);

    // On cherche le dernier séparateur de notre chemin absolu
    char* lastSeparator = strrchr(dossier_de_base, file_separator[0]);

    // S'il y en a un
    if (lastSeparator != NULL) {
        // on retire juste le dernier nom de dossier qu'on a pu mettre
        dossier_de_base[strlen(dossier_de_base) - (strlen(lastSeparator) - 1)] = '\0';
    } else {
        // Sinon c'est que le chemin n'existe pas
        printf("Could not find '%s' on your system.", dossier_de_base);
        return;
    }

    // On lance notre boucle qui s'occupera de renvoyer les fichiers finaux à cmd_import avec les bonnes informations
    startListingFiles(za, chemin_absolu, dossier_de_base, prefix);
    free(dossier_de_base);
    free(chemin_absolu);
}


// Code de la commande "rm"
void cmd_rm_recursive(struct zip *za, const char *filename_in_zip, const char *prefix) {
    
    char dir_name_with_slash[256];
    // Copie le nom du repertoire dans dir_name_with_slash
    strncpy(dir_name_with_slash, filename_in_zip, sizeof(dir_name_with_slash) - 1);
    // S'assure que la chaine se termine par '\0'
    dir_name_with_slash[sizeof(dir_name_with_slash) - 1] = '\0';

    // Si le dernier caractere n'est pas un '/', alors on ajoute un '/'
    if (dir_name_with_slash[strlen(dir_name_with_slash) - 1] != '/') {
        strncat(dir_name_with_slash, "/", sizeof(dir_name_with_slash) - strlen(dir_name_with_slash) - 1);
    }

    if (dir_exists(za, dir_name_with_slash, prefix)) {

        // On met en une seule variable prefix suivi de dir_name_with_slash
        char new_prefix[256];
        snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, dir_name_with_slash);

        // On recupere le nombre d'occurrence qui commencent par new_prefix
        int file_count;
        const char **files = list_files(za, new_prefix, &file_count);
        
        // Pour chaque fichier dans la liste, on supprime le fichier de l'archive zip
        for (int i = 0; i < file_count; i++) {
            const char *filename = files[i];
            
            for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
                struct zip_stat sb;
                zip_stat_index(za, i, 0, &sb);

                if (strcmp(sb.name, filename) == 0) {
                    if (cmd_rm(za, i) != ZIP_ER_OK) {
                        printf("Failed to delete file '%s'\n", filename);
                        return;
                    }

                    if (verbose == 1) {
                        printf("File '%s' deleted successfully\n", filename);
                    }
                }
            }
        }

        // Apres avoir supprime les fichiers, on verifie que le dossier soit bien vide, et on le supprime si c'est vide
        for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
            struct zip_stat sb;
            zip_stat_index(za, i, 0, &sb);

            if (strcmp(sb.name, new_prefix) == 0) {
                int delete_status = cmd_rm(za, i);
                if (delete_status == ZIP_ER_OK && verbose == 1) {
                    printf("File '%s' deleted successfully\n", dir_name_with_slash);
                }
            }
        }

        free(files);

    } else {

        // On met en une seule variable prefix suivi de filename_in_zip
        char new_prefix[256];
        snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, filename_in_zip);

        // On fait une boucle pour sortir tout le zip du zip
        for (int i = 0; i < zip_get_num_entries(za, 0); i++) {
            struct zip_stat sb;
            zip_stat_index(za, i, 0, &sb);

            // On check si le nom du fichier est celui qu'on veut supprimer
            if (strcmp(sb.name, new_prefix) == 0) {
                // On le supprime si c'est le cas
                if (cmd_rm(za, i) != ZIP_ER_OK) {
                    printf("Failed to delete file '%s'\n", new_prefix);
                    return;
                }
                if (verbose == 1) {
                    printf("File '%s' deleted successfully\n", new_prefix);
                }
                return; // On quitte la fonction vu qu'on a supprime le fichier
            }
        }
        printf("File '%s' not found\n", new_prefix);
    }
}
