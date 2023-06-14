#include <zip.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <libgen.h>
#include <string.h>

#include "shell_dependencies.h"

extern int verbose;

// declaration des macro preprocesseurs
#if defined(_WIN32) || defined(_WIN64)
    #include <sys/types.h>
    #include <direct.h>
    #include <io.h>
    #define mkdir(dir) _mkdir(dir)
    #define access(dir) _access(dir, 0)
    const char *file_separator = "\\";
#else
    #include <linux/limits.h>
    #include <unistd.h>
    #include <dirent.h>
    #define mkdir(dir) mkdir(dir, 0700)
    #define access(dir) access(dir, F_OK)
    const char *file_separator = "/";
#endif


// cmd_cd
// cmd_export_recursive
// cmd_rm_recursive
// Code qui verifie l'existance d'un dossier dans la logique de ls
int dir_exists(struct zip *za, const char *dir_name, const char *prefix) {
    int num_files = zip_get_num_entries(za, 0);
    size_t prefix_len = strlen(prefix);

    char dir_name_with_slash[256];
    // Copie le nom du repertoire dans dir_name_with_slash
    strncpy(dir_name_with_slash, dir_name, sizeof(dir_name_with_slash) - 1);
    // S'assure que la chaine se termine par '\0'
    dir_name_with_slash[sizeof(dir_name_with_slash) - 1] = '\0';

    // Si le dernier caractere n'est pas un '/', alors on ajoute un '/'
    if (dir_name_with_slash[strlen(dir_name_with_slash) - 1] != '/') {
        strncat(dir_name_with_slash, "/", sizeof(dir_name_with_slash) - strlen(dir_name_with_slash) - 1);
    }

    char new_prefix[256];
    snprintf(new_prefix, sizeof(new_prefix), "%s%s", prefix, dir_name_with_slash);

    for (int i = 0; i < num_files; i++) {
        struct zip_stat sb;
        zip_stat_index(za, i, 0, &sb);

        // Si l'entree commence par new_prefix, alors le dossier existe
        if (strncmp(sb.name, new_prefix, strlen(new_prefix)) == 0) {
            return 1;
        }
    }

    // Si aucune entrée ne commence par new_prefix, alors le dossier n'existe pas
    return 0;
}


// cmd_export_recursive
// Code pour remplacer un caractère par un autre (comme / vers \ )
char* replace_string(const char* str, const char* find, const char* replace_str) {
    char *new_str = strdup(str);
    int len  = strlen(new_str);
    int len_find = strlen(find);
    int len_replace = strlen(replace_str);
    for (char* ptr = new_str; ptr = strstr(ptr, find); ++ptr) {
        if (len_find != len_replace) 
            memmove(ptr+len_replace, ptr+len_find,
                len - (ptr - new_str) + len_replace);
        memcpy(ptr, replace_str, len_replace);
    }
    return new_str;
}


// cmd_export_recursive
// Commande qui sert a eliminer le prefixe avant que nous le passions dans cmd_export (utile dans le cas ou on exporte un sous-dossier uniquement)
const char* remove_prefix(const char* object_name, const char* prefix) {
    size_t len = strlen(prefix);
    prefix = replace_string(prefix, "/", file_separator);
    if (strncmp(object_name, prefix, len) == 0) {
        return object_name + len;
    } else {
        return object_name;
    }
}


// cmd_export_recursive
// Code execute pour effectuer un export d'un fichier
void cmd_export(struct zip *za, const char *filename_in_zip, const char *destination_path, char* prefix) {
    struct zip_stat sb;
    zip_stat_init(&sb);
    zip_stat(za, filename_in_zip, 0, &sb);

    // On remplace les "\" mis precedement par des "/" pour libzip
    char *file_in_zip_for_libzip = replace_string(filename_in_zip, "\\", "/");

    // On verifie si le prefix est present dans le nom de fichier (utile si on est dans un sous-dossier)
    // si c'est pas le cas, on l'ajoute devant, sinon on laisse tel quel
    char file_with_prefix[256];
    if (strncmp(file_in_zip_for_libzip, prefix, strlen(prefix)) == 0) {
        strncpy(file_with_prefix, file_in_zip_for_libzip, sizeof(file_with_prefix));
        file_with_prefix[sizeof(file_with_prefix) - 1] = '\0';
    } else {
        snprintf(file_with_prefix, sizeof(file_with_prefix), "%s%s", prefix, file_in_zip_for_libzip);
    }

    // on ouvre le fichier dans le zip
    struct zip_file *zf = zip_fopen(za, file_with_prefix, 0);
    if (!zf) {
        printf("Could not open file '%s' in zip\n", file_with_prefix);
        free(file_in_zip_for_libzip);
        return;
    }

    // on ouvre / cree un fichier de destination
    char complete_destination_path[1024];
    const char *filename_export = remove_prefix(filename_in_zip, prefix);
    snprintf(complete_destination_path, sizeof(complete_destination_path), "%s%s", destination_path, filename_export);

    FILE *destination_file = fopen(complete_destination_path, "wb");
    if (!destination_file) {
        printf("Could not open destination file '%s'\n", complete_destination_path);
        zip_fclose(zf);
        return;
    }

    // on copie le fichier
    char buffer[8192];
    zip_int64_t nbytes;
    while ((nbytes = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, nbytes, destination_file);
    }

    // on ferme tout
    fclose(destination_file);
    zip_fclose(zf);
    free(file_in_zip_for_libzip);
    if (verbose == 1) {
        printf("Exported file '%s' to '%s'\n", filename_in_zip, destination_path);
    }
}


// cmd_export_recursive
// Code de test pour checker si on peut créer un dossier
void create_dir(const char *dir) {
    if (access(dir) == -1) {
        if (mkdir(dir) == 0) {
            if (verbose == 1) {
                printf("Created directory '%s'\n", dir);
            }
        } else {
            printf("Unable to create directory '%s'\n", dir);
        }
    }
}


// cmd_export_recursive
// cmd_rm_recursive
// Code qui sert à récupérer la liste des fichiers dans la logique de ls, mais avec la répétition pour chaque objet
const char **list_files(struct zip *za, const char *prefix, int *printed_items_count) {
    
    size_t prefix_len = strlen(prefix); // Recupere la longueur du préfixe (nom du dossier actuel via la commande "cd")
    struct zip_stat sb;
    const char **printed_items = NULL; // Initialisation du tableau qui va contenir les noms de dossiers
    *printed_items_count = 0; // Compteur du nombre de dossiers deja affiches

    int num_files = zip_get_num_entries(za, 0);
    for (int i = 0; i < num_files; i++) {
        zip_stat_index(za, i, 0, &sb);

        if (strncmp(sb.name, prefix, prefix_len) == 0) {
            printed_items = realloc(printed_items, sizeof(char*) * (*printed_items_count + 1)); // On agrandit notre tableau dynamique
            printed_items[*printed_items_count] = sb.name; // On ajoute le nom du fichier a notre tableau
            (*printed_items_count)++; // On incremente notre compteur de dossiers affiches
        }
    }

    // Liberation de la memoire allouee pour le tableau et ses elements
    return printed_items;
}


// cmd_import_recursive
// Code qui sert à convertir un char* en zip_source_t*. Effectue egalement une verification de l'existance du fichier
zip_source_t* get_file_data(const char* filename) {
    zip_error_t err;
    zip_source_t* s = zip_source_file_create(filename, 0, -1, &err);
    if (s == NULL) {
        printf("Failed to create data source from file '%s' : %s\n", filename, zip_error_strerror(&err));
    }
    return s;
}


// cmd_import_recursive
// Code qui sert a obtenir un chemin absolu par rapport à un chemin relatif
char* getAbsolutePath(char* relativePath) {
    #if defined(_WIN32) || defined(_WIN64)
        char* absolutePath = malloc(1000 * sizeof(char));
    #else
        char* absolutePath = malloc(PATH_MAX * sizeof(char));
    #endif
    if(absolutePath == NULL) {
        printf("Failed to allocate memory.\n");
        return NULL;
    }
    #if defined(_WIN32) || defined(_WIN64)
        if(_fullpath(absolutePath, relativePath, 1000) == NULL)
    #else
        if(realpath(relativePath, absolutePath) == NULL)
    #endif
    {
        printf("Error when trying to convert relative path to absolute path.\n");
        free(absolutePath);
        return NULL;
    }

    return absolutePath;
}


// cmd_import_recursive
// Cpde qui sert à supprimer la partie absolu du dossier de base fourni
char* removeSubstring(char* important, char* a_supprimer) {
    char *p;
    p = strstr(important, a_supprimer);

    if(p) {
        char* nouveau_chemin = malloc(strlen(important) - strlen(a_supprimer) + 1);
        if(nouveau_chemin == NULL) {
            printf("Failed to allocate memory\n");
            exit(1);
        }

        strcpy(nouveau_chemin, p + strlen(a_supprimer));
        return nouveau_chemin;
    } else {
        printf("Substring not found!\n");
        return NULL;
    }
}


// cmd_import_recursive
// Code execute pour effectuer un import d'un fichier
void cmd_import(struct zip* za, const char* filename, char *original_directory, char* prefix) {
    int err;

    // Créer le nom du fichier avec le prefixe
    char file_with_prefix[256];
    if (original_directory != NULL) {
        char *filename_copy = strdup(filename);
        char *a_garder = removeSubstring(filename_copy, original_directory);
        char *file_in_zip_for_libzip = replace_string(a_garder, file_separator, "/");
        
        snprintf(file_with_prefix, sizeof(file_with_prefix), "%s%s", prefix, file_in_zip_for_libzip);
        
        free(a_garder);
        free(file_in_zip_for_libzip);
        free(filename_copy);
    } else {
        printf("Unable to find last directory name in '%s'\n", filename);
        free(original_directory);
        return;
    }
    
    // Obtenir les données du fichier à importer
    zip_source_t* s = get_file_data(filename);
    
    // Si les données du fichier n'ont pas pu être lues, afficher un message d'erreur et quitter la fonction
    if (s == NULL) {
        printf("Unable to read data from file '%s'\n", filename);
        return;
    }
    
    // Ajouter le nouveau fichier à l'archive ZIP
    zip_int64_t index = zip_file_add(za, file_with_prefix, s, ZIP_FL_ENC_UTF_8);
    
    // Si l'ajout du fichier à échoué, afficher un message d'erreur et quitter la fonction
    if (index == -1) {
        printf("Failed to add file '%s' to ZIP: %s\n", file_with_prefix, zip_strerror(za));
        zip_source_free(s);
        return;
    }
    if (verbose == 1) {
        printf("File '%s' have successfully been imported in '%s'\n", filename, file_with_prefix);
    }
}


// cmd_import_recursive
// Code qui supprime les espaces et les file_separator en fin de chaine de caractères
void removeTrailingFileSeparator(char *path) {
    int length = strlen(path);
    while(length > 0 && (path[length - 1] == file_separator[0] || path[length - 1] == ' ')) {
        path[length - 1] = '\0';
        length--;
    }
}


// cmd_import_recursive
// Code qui enumere tous les fichiers d'un dossier (ou le fichier seul). Une fonction par OS
#if defined(_WIN32) || defined(_WIN64)
void startListingFiles(struct zip *za, const char *basePath, char *content_to_remove, char *prefix)
{
    struct _finddata_t data;
    char path[1000];
    char originalPath[1000];
    strcpy(originalPath, basePath);

    struct _stat statbuf;
    if (_stat(originalPath, &statbuf) != 0) {
        printf("Could not find '%s'\n", basePath);
        return;
    }

    // Si c'est un fichier et non un répertoire.
    if (statbuf.st_mode & _S_IFREG) {
        cmd_import(za, originalPath, content_to_remove, prefix);
        return;
    }

    long handle = _findfirst(strcat(strcpy(path, basePath), "\\*"), &data); // commencer a chercher

    if(handle == -1L) return;

    do {
        if (strcmp(data.name, ".") != 0 && strcmp(data.name, "..") != 0)
        {
            // Construire un nouveau chemin a partir de notre chemin de base
            strcpy(path, originalPath);
            strcat(path, file_separator);
            strcat(path, data.name);

            if(!(data.attrib & _A_SUBDIR)) // Vérifier si l'element est un fichier
            {
                cmd_import(za, path, content_to_remove, prefix);
            }
            else // Si c'est un repertoire, continuer la récursion
            {
                startListingFiles(za, path, content_to_remove, prefix);
            }
        }
    } while(_findnext(handle, &data) == 0);

    _findclose(handle); // fermer le handle
}
#else
void startListingFiles(struct zip *za, const char *basePath, char *content_to_remove, char *prefix)
{
    char path[1000];
    struct dirent *dp;
    struct stat statbuf;

    // Si le fichier existe
    if(stat(basePath, &statbuf) != -1){
        // Si c'est un fichier
        if(S_ISREG(statbuf.st_mode)){
            cmd_import(za, basePath, content_to_remove, prefix);
            return;
        }
    } else {
        printf("Could not find '%s'\n", basePath);
        return;
    }

    DIR *dir = opendir(basePath);

    // Si on ne peut pas ouvrir le repertoire
    if (!dir) return;

    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
        {
            // Construire un nouveau chemin a partir de notre chemin de base
            sprintf(path, "%s/%s", basePath, dp->d_name);

            if(stat(path, &statbuf) != -1){
                // Si c'est un repertoire, on continue la recursion
                if(S_ISDIR(statbuf.st_mode)){
                    startListingFiles(za, path, content_to_remove, prefix);
                }
                // Si c'est un fichier, on l'importe
                else if(S_ISREG(statbuf.st_mode)){
                    cmd_import(za, path, content_to_remove, prefix);
                }
            }
        }
    }

    closedir(dir);
}
#endif


// cmd_rm_recursive
int cmd_rm(struct zip *za, int index) {
    if (zip_delete(za, index) == -1) {
        // Get the libzip error code and return it
        zip_error_t *error = zip_get_error(za);
        return zip_error_code_zip(error);
    }

    // Return ZIP_ER_OK if file was successfully deleted
    return ZIP_ER_OK;
}