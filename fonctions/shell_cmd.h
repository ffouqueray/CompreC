// Declaration des fonctions de shell_cmd.c
void cmd_help();

void cmd_ls(struct zip *, const char *);

void cmd_cd(struct zip *, const char *, char *);

void cmd_export_recursive(struct zip *, const char *, const char *, char *);

void cmd_import_recursive(struct zip *, char *, char *);

void cmd_rm_recursive(struct zip *, const char *, const char *);