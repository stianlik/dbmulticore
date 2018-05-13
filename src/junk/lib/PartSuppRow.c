#include "PartSuppRow.h"
#include "constants.h"
#include "db_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PartSuppRow *PartSuppRow_read(char *buffer) {
	struct PartSuppRow *row = malloc(sizeof(struct PartSuppRow));
	row->partkey = db_int(strtok(buffer, SEPARATOR));
	row->suppkey = db_int(strtok(NULL, SEPARATOR));
	row->availqty = db_int(strtok(NULL, SEPARATOR));
	row->supplycost = db_string(strtok(NULL, SEPARATOR));
	row->comment = db_string(strtok(NULL, SEPARATOR));
	return row;
}

void PartSuppRow_free(struct PartSuppRow *row) {
	free(row->supplycost);
	free(row->comment);
	free(row);
}

void PartSuppRow_print(struct PartSuppRow *row) {
	printf("|%d|%d|\n", row->partkey, row->suppkey);
}
