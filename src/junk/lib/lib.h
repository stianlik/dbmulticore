#ifndef DB_LIB_GUARD
#define DB_LIB_GUARD

#define ROW_READER(A) (void *(*)(char*)) A

#include <time.h>
#include <stdio.h>

// Linked list
// TODO Move to separate file

struct Node {
	void *data;
	void *next;
};
struct Node *ll_node(struct Node *prev, void *data);
int ll_length(struct Node* header);

// Database functions

FILE *open_table(char name[]);
struct Node *db_load_table(char name[], void *(*read_row)(char*));
//void db_free_table(struct Node *head, void (*)(void*));

// Benchmarking
// TODO Benchmarking file

int sched_getcpu(void); 
void delay(time_t s, long ns);

#endif
