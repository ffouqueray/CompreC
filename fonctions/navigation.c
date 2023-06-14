#include <zip.h>
#include <stdio.h>
#include <string.h>

#include "navigation.h"
#include "shell_cmd.h"
#include "colors.h"

char *global_pwd = NULL;
int verbose = 0;

// Notre shell interactif qui permet de taper les commandes
void navigate(struct zip *za, char *archive) {
    char command[256];
    char prefix[256] = "";

    if (global_pwd != NULL) {
        printf("\nBecause libzip has known issues with encrypted zip, you are not able to use edition commands (\"rm\" and \"import\") on encrypted zip files.\nTo use these commands, you have to clone the zip but decrypted.\n");
    }

    while (1) {
        printf("\n%s @ %s\n$ ", archive, prefix[0] ? prefix : "/");
        fgets(command, 256, stdin);

        // on retire le retour a la ligne
        command[strcspn(command, "\n")] = 0;


        // Si la commande tapee est "help"
        if (strcmp(command, "help") == 0) {
            cmd_help(za, (char *) prefix);
        }

        // Si la commande tapee est "ls"
        else if (strcmp(command, "ls") == 0 || strcmp(command, "ll") == 0 || strcmp(command, "l") == 0) {
            cmd_ls(za, (char *) prefix);
        }

        // Si la commande tapee est "cd"
        else if (strcmp(command, "cd") == 0) { // Si la commande tapee est seule (sans argument)
            if (strlen(command) == 2) {
                prefix[0] = '\0'; // vide le prefixe
            }
        } else if (strncmp(command, "cd ", 3) == 0) { // S'il y a un argument de tape avec
            const char *new_dir = command + 3;
            cmd_cd(za, new_dir, prefix);
        }

        // Si la commande tapee est "export"
        else if (strncmp(command, "export ", 7) == 0) {
            const char *filename_in_zip = command + 7;
            char *separator = strchr(filename_in_zip, '*');
            if (separator) {
                // Cut the string at the separator and get the destination path
                *separator = '\0';
                const char *destination_path = separator + 1;
                cmd_export_recursive(za, filename_in_zip, destination_path, prefix);
            } else {
                printf("Invalid command format\nRefer to help command to get the right format\n");
            }
        }

        // Si la commande tapee est "import"
        else if (strncmp(command, "import ", 7) == 0) {
            if (global_pwd != NULL) {
                printf("You can not import into an encrypted zip.\n");
            } else {
                char *filename = command + 7;
                cmd_import_recursive(za, filename, prefix);
            }
        }

        // Si la commande tapee est "rm"
        else if (strncmp(command, "rm ", 3) == 0) {
            if (global_pwd != NULL) {
                printf("You can not remove from an encrypted zip.\n");
            } else {
                char *filename = command + 3;
                cmd_rm_recursive(za, filename, prefix);
                printf("\nDue to limitation of the zip library, the archive will now close.\n\n");
                break;
            }
        }
        
        // Si la commande tapee est "exit"
        else if (strcmp(command, "exit") == 0) {
            if (verbose == 1) {
                printf("exiting...\n");
            }
            break;
        }

        // Si on ne connait pas la commande tapee
        else {
            printf("Unknown command: '%s'\nRefer to help command to get the right format\n", command);
        }
    }
}