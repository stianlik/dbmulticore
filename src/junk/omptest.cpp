#include <omp.h>
#include <iostream>
#include <stdlib.h>
#include <ctime>

#define TUPLE_COUNT 240000000

using namespace std;
int main() {
	/*
	#pragma omp parallel for
	for (int i = 0; i < 1000000000; i++) {
	}
	cout << "Done with " << endl;
	*/

	// Generate test data
	int *table = new int[TUPLE_COUNT];
	for (int i = 0; i < TUPLE_COUNT; ++i) {
		table[i] = rand();
	}

	// Select values
	double start = omp_get_wtime();
	int *result = new int[TUPLE_COUNT];
	int j = 0;
	#pragma omp parallel for default(none) shared(table, result, j)
	for (int i = 0; i < TUPLE_COUNT; ++i) {
		if (table[i] == 3) {
			result[j++] = table[i];
		}
	}
	double end = omp_get_wtime();
	cout << "Select: " << end - start << endl;

	return 0;
}
