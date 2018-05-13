#include "PartRow.h"
#include "constants.h"
#include "db_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PartRow *PartRow_read(char *buffer) {
	struct PartRow *row = malloc(sizeof(struct PartRow));
	row->partkey = db_int(strtok(buffer, SEPARATOR));
	row->name = db_string(strtok(NULL, SEPARATOR));
	row->mfgr = db_string(strtok(NULL, SEPARATOR));
	row->brand = db_string(strtok(NULL, SEPARATOR));
	row->type = db_string(strtok(NULL, SEPARATOR));
	row->size = db_int(strtok(NULL, SEPARATOR));
	row->container = db_string(strtok(NULL, SEPARATOR));
	row->retailprice = db_string(strtok(NULL, SEPARATOR));
	row->comment = db_string(strtok(NULL, SEPARATOR));
	return row;
}

void PartRow_free(struct PartRow *row) {
	free(row->name);
	free(row->mfgr);
	free(row->brand);
	free(row->type);
	free(row->container);
	free(row->retailprice);
	free(row->comment);
	free(row);
}

void PartRow_print(struct PartRow *row) {
	printf("|%d|%s|\n", row->partkey, row->name);
}
