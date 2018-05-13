#include "db_types.h"
#include <stdlib.h>
#include <string.h>

int db_int(char *token) {
	return atoi(token);
}

char *db_string(char *token) {
	char *str = malloc(sizeof(*str)*256); // TODO waste of memory
	strcpy(str, token);
	return str;
}
