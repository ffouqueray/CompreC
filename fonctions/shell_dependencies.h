// Declaration d'une macro preprocesseur dans shell_dependencies.c utilisee egalement dans shell_cmd.c
extern const char *file_separator;

// Declaration des fonctions de shell_dependencies.c
int dir_exists(struct zip *, const char *, const char *);

char* replace_string(const char *, const char *, const char *);

const char* remove_prefix(const char *, const char *);

void cmd_export(struct zip *, const char *, const char *, char *);

void create_dir(const char *);

const char **list_files(struct zip *, const char *, int *);

zip_source_t *get_file_data(const char *);

void removeTrailingFileSeparator(char *);

char* getAbsolutePath(char *);

void startListingFiles(struct zip *, const char *, char *, char *);

int cmd_rm(struct zip *, int);