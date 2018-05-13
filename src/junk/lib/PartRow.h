#ifndef PARTROW_GUARD
#define PARTROW_GUARD

struct PartRow {
	int partkey;
	char *name;
	char *mfgr;
	char *brand;
	char *type;
	int size;
	char *container;
	char *retailprice;
	char *comment;
};
struct PartRow *PartRow_read(char *buffer);
void PartRow_free(struct PartRow *row);
void PartRow_print(struct PartRow *row);

#endif
