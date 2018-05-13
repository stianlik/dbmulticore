#define LINE_LENGTH 256
#define SEPARATOR "|"

int db_int(char*);
char *db_string(char*);
struct Node *db_load_table(char name[], void *(*read_row)(char*));
void db_free_table(struct Node *head);
