#include "util.cpp"
using namespace std;

void select_solution(row16_t *table, int tableLength, bool (*condition)(row16_t&), vector<row16_t> &result) {
	for (int i = 0; i < tableLength; i++) {
		if (condition(table[i])) {
			result.push_back(table[i]);
		}
	}
}

void select_omp(row16_t *table, int tableLength, bool (*condition)(row16_t&), row16_t *result, int &resultLength) {
	resultLength = 0;

	#pragma omp parallel for
	for (int i = 0; i < tableLength; i++) {
		if (condition(table[i])) {
			#pragma omp critical
			result[resultLength++] = table[i];
		}
	}
}
