#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lib/lib.h"
#include "lib/HashTable.h"
#include "lib/PartRow.h"
#include "lib/PartSuppRow.h"

//#define DEBUG

#define BUCKETSIZE 5

int h(int pk, int bucketcount) {
	return pk % bucketcount;
}

int main(int argc, char* argv[]) {

	// Read tables from filesystem
	struct Node *part_table = db_load_table("data/part.tbl", ROW_READER(&PartRow_read));
	struct Node *partsupp_table = db_load_table("data/partsupp.tbl", ROW_READER(&PartSuppRow_read));

	printf("Tables in memory\n");

	// Build
	HashTable *table = HashTable_create(ll_length(part_table) * 1.25f, BUCKETSIZE);
	struct Node *cur = part_table;
	int key;
	while( (cur = cur->next) != NULL) {
		key = h( ((struct PartRow*) cur->data)->partkey, table->bucketcount);
		HashTable_put(table, key, cur->data);
	}

	printf("Hash table built\n");

	// Probe
	cur = partsupp_table;
	struct PartRow *a;
	struct PartSuppRow *b;
	struct Node *result = ll_node(NULL,NULL);
	struct Node *cur_result = result;
	while( (cur = cur->next) != NULL) {
		b = (struct PartSuppRow*) cur->data;
		key = h( ((struct PartSuppRow*) cur->data)->partkey, table->bucketcount);
		HashBucket *bucket = HashTable_get(table,key);
		if (bucket->next_index > 0) {
			for (int i = 0; i < bucket->next_index; i++) {
				a = (struct PartRow*) bucket->values[i];
				if (a->partkey == b->partkey) {
					// Save to result
					void **data = malloc(sizeof(void*) * 2);
					data[0] = a;
					data[1] = b;
					cur_result = ll_node(cur_result, data);
#ifdef DEBUG
					printf("Combine %d with %d\n", ((struct PartRow*)data[0])->partkey, ((struct PartSuppRow*)data[1])->suppkey);
#endif
					break;
				}
			}
		}
	}

	printf("Probing completed\n");

	return 0;
}
