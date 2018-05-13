#include "HashTable.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

HashTable *HashTable_create(int size, int bucketsize) {
	HashTable *table = malloc( sizeof(*table) );
	table->bucketcount = ceil( (float) size / (float) bucketsize);
	table->buckets = malloc( sizeof(HashBucket*) * table->bucketcount );
	table->bucketsize = bucketsize;

	for (int i = 0; i < table->bucketcount; i++) {
		table->buckets[i] = malloc(sizeof(HashBucket));
		table->buckets[i]->values = malloc(sizeof(void*) * bucketsize);
		table->buckets[i]->next_index = 0;
		table->buckets[i]->overflow = NULL;
	}

	return table;
}

void HashTable_put(HashTable *table, int key, void *value) {
	HashBucket *bucket = HashTable_get(table, key);

	if (bucket->next_index < table->bucketsize) {
		bucket->values[bucket->next_index] = value;
		bucket->next_index++;
	}
	else {
		// Overflow
		printf("HashBucket overflow not implemented: %d\n", bucket->next_index);
		exit(1);
	}
}

HashBucket *HashTable_get(HashTable *table, int key) {
	if (key < table->bucketcount) {
		return table->buckets[key];
	}
	
	printf("HashTable overflow not implemented: %d\n", key);
	exit(1);
}
