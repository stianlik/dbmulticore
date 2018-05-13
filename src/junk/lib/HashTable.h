#ifndef HASHTABLE_GUARD
#define HASHTABLE_GUARD

typedef struct HashBucket {
	int next_index;
	struct HashBucket *overflow;
	void **values;
} HashBucket;

typedef struct HashTable {
	int bucketcount;
	int bucketsize;
	HashBucket **buckets;
} HashTable;

HashTable *HashTable_create(int size, int bucketsize);
void HashTable_put(HashTable*,int key, void *value);
HashBucket *HashTable_get(HashTable*,int key);

#endif
