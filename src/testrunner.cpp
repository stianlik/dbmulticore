#define JOINVAR_LEFT key
#define JOINVAR_RIGHT key
#define JOINOP ==
#define BUCKET_SIZE 4

#define _FILEPATH(T) "../data/cur/" # T ".tbl"
#define FILEPATH(T) _FILEPATH(T)

#include <omp.h>
#include "hashjoin.cpp"
#include "hashjoin_omp.cpp"
#include "PerfCounter.cpp"
#include "select.cpp"

using namespace std;

void assertTableEquals(row16_t *expected, row16_t *actual, int length) {
	for (int i = 0; i < length; i++) {
		assert(expected[i].key == actual[i].key);
		assert(expected[i].payload == actual[i].payload);
	}
}

bool selectCond1(row16_t &a) {
	return a.key <= 5 ;
}

int main(int argc, char* argv[]) {

	string test = (argc > 1) ? argv[1] : "hashjoin";
	int threadCounts[] = {1,2,4,6,8,12,24,32,64,128};
	int resultLength;
	const int threadCountsLength = 10;
	double startClock, endClock;
	vector<row16_t> tableLeft;
	vector<row16_t> tableRight;
	vector<row32_t> result;
	Perfcounter stats;

	startClock = omp_get_wtime();

	// Read tables into memory
	tableLeft.reserve(25 * 1024 * 1024);
	tableRight.reserve(100 * 1024 * 1024);
	cout << "# Reading tables" << endl;
	readTable<row16_t>(FILEPATH(part), tableLeft);
	readTable<row16_t>(FILEPATH(partsupp), tableRight);
	cout << "# Performing tests for " << test << endl;

	// Run tests
	for (int i = 0; i < 10; i++) {
		result.clear();

		// Hash join (sequential)
		if (test == "hashjoin") {
			result.reserve(tableRight.size());
			stats.start();
			omp_set_num_threads(1);
			hashjoin(
				&tableLeft[0], tableLeft.size(), 
				&tableRight[0], tableRight.size(), 
				&result[0], resultLength
			);
			stats.checkpoint();

#ifdef DEBUG
			for (int j = 0; j < (int) resultLength; j++) {
				cout << result[j].left.key << "|" << result[j].left.payload << "|" << result[j].right.key << "|" << result[j].right.payload << endl;
			}
#endif
		}

		// Hash join (parallel)
		else if (test == "hashjoin_omp") {
			result.reserve(tableRight.size());
			stats.start();
			for (int j = 0; j < threadCountsLength; j++) {
				omp_set_num_threads(threadCounts[j]);
				hashjoin_omp(
					&tableLeft[0], static_cast<int>(tableLeft.size()), 
					&tableRight[0], static_cast<int>(tableRight.size()), 
					&result[0], resultLength
				);
				assert(resultLength == static_cast<int>(tableRight.size()));
				stats.checkpoint();
			}
		}

		// Select (parallel)
		else if (test == "select_omp") {
			// Generate correct solution
			vector<row16_t> expected;
			select_solution(&tableLeft[0], tableLeft.size(), selectCond1, expected);

			// Prepare result
			row16_t *result = new row16_t[expected.size()];

			// Test parallel method
			stats.start();
			for (int j = 0; j < threadCountsLength; j++) {
				omp_set_num_threads(threadCounts[j]);
				select_omp(&tableLeft[0], tableLeft.size(), selectCond1, &result[0], resultLength);
				assert(static_cast<int>(expected.size()) == resultLength);
				stats.checkpoint();
			}
			delete[] result;
		}

		// Not found
		else {
			cout << "# Error: Trying to run non-existing test" << endl;
			exit(1);
		}

	}

	// Summary
	endClock = omp_get_wtime();
	cout << "# Total time in algorithm: " << stats.totalTime << " ms" << endl;
	cout << "# Ellapsed time: " << endClock - startClock << " seconds" << endl;

	return 0;
}
