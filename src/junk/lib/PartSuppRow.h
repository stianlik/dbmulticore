#ifndef PARTSUPPROW_GUARD
#define PARTSUPPROW_GUARD

struct PartSuppRow {
	int partkey;
	int suppkey;
	int availqty;
	char *supplycost;
	char *comment;
};
struct PartSuppRow *PartSuppRow_read(char *buffer);
void PartSuppRow_free(struct PartSuppRow *row);
void PartSuppRow_print(struct PartSuppRow *row);

#endif
