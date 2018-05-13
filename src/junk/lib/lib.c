#include "constants.h"
#include "lib.h"
#include <stdlib.h>
#include <sched.h>

// Linked list

struct Node *ll_node(struct Node *prev, void *data) {
	struct Node *node;
	node = malloc(sizeof(*node));
	if (prev != NULL) {
		prev->next = node;
	}
	if (data != NULL) {
		node->data = data;
	}
	return node;
}

int ll_length(struct Node *header) {
	return  *((int*)header->data);
}

// Database functions

FILE *open_table(char name[]) {
	FILE *fp = fopen(name, "r");

	if (fp == NULL) {
		printf("unable to load table: %s \n", name);
		exit(1);
	}

	return fp;
}

struct Node *db_load_table(char name[], void *(*read_row)(char*)) {
	FILE *fp = open_table(name);
	char *buffer;
	struct Node *head, *cur;

	buffer = malloc(sizeof(*buffer) * LINE_LENGTH);
	head = ll_node(NULL, malloc(sizeof(int)));
	cur = head;

	while (fgets(buffer, LINE_LENGTH, fp) != NULL) {
		(*((int*)head->data))++;
		cur = ll_node(cur, read_row(buffer));
	}

	free(buffer);
	fclose(fp);

	return head;
}

// TODO Generic table deallocation

// Benchmarking

void delay(time_t s, long ns) {
	struct timespec test;
	test.tv_sec = s;
	test.tv_nsec = ns;
	nanosleep(&test, &test);
}

