#include <cstdlib>
#include <cstdio>
#include <algorithm>
#include <climits>
#include <cstring>
#include <cstdio>
#include <pthread.h>
#include <cstdarg>
#include "read_table.cpp"
#include "tictoc.cpp"
#include "threadpool.cpp"
#define DEBUG 0

/**
 * This file defines a number of tests for the threshold
 * algorithm. Additionaly, sequential TA is implemented with
 * name ta(). Binary search to find insertion points
 * are included as bin_search. Tests are defined as functions:
 * test1(), test2(), etc.
 */

struct score_t {
	int key;
	double value;
};

struct result_t {
	int size;
	int capacity;
	score_t *rows;
};

void result_init(int k, result_t &result) {
	result.rows = new score_t[k];
	result.capacity = k;
	result.size = 0;
}

void result_destroy(result_t &result) {
	delete[] result.rows;
}

// Scoring functions

double *weights = new double[32] {
	0.3, 0.2, 0.1, 0.3, 0.5, 0.3, 0.2, 0.1,
	0.5, 0.3, 0.2, 0.1, 0.5, 0.3, 0.2, 0.1,
	0.5, 0.3, 0.2, 0.1, 0.5, 0.3, 0.2, 0.1,
	0.5, 0.3, 0.2, 0.1, 0.5, 0.3, 0.2, 0.1
};
double weights_length;

double f_2(int *row) {
	return 
		0.5 * (double) row[0] +
		0.5 * (double) row[1];
}

double f_4(int *row) {
	return 
		0.3 * (double) row[0] +
		0.2 * (double) row[1] +
		0.1 * (double) row[2] +
		0.3 * (double) row[3];
}

double f_8(int *row) {
	return 
		0.3 * (double) row[0] +
		0.2 * (double) row[1] +
		0.1 * (double) row[2] +
		0.3 * (double) row[3] +
		0.3 * (double) row[4] +
		0.2 * (double) row[5] +
		0.1 * (double) row[6] +
		0.3 * (double) row[7];
}

double f_16(int *row) {
	return 
		0.3 * (double) row[0] +
		0.2 * (double) row[1] +
		0.1 * (double) row[2] +
		0.3 * (double) row[3] +
		0.3 * (double) row[4] +
		0.2 * (double) row[5] +
		0.1 * (double) row[6] +
		0.3 * (double) row[7] +
		0.3 * (double) row[8] +
		0.2 * (double) row[9] +
		0.1 * (double) row[10] +
		0.3 * (double) row[11] +
		0.3 * (double) row[12] +
		0.2 * (double) row[13] +
		0.1 * (double) row[14] +
		0.3 * (double) row[15];
}

double f_32(int *row) {
	return 
		0.3 * (double) row[0] +
		0.2 * (double) row[1] +
		0.1 * (double) row[2] +
		0.3 * (double) row[3] +
		0.3 * (double) row[4] +
		0.2 * (double) row[5] +
		0.1 * (double) row[6] +
		0.3 * (double) row[7] +
		0.3 * (double) row[8] +
		0.2 * (double) row[9] +
		0.1 * (double) row[10] +
		0.3 * (double) row[11] +
		0.3 * (double) row[12] +
		0.2 * (double) row[13] +
		0.1 * (double) row[14] +
		0.3 * (double) row[15] +
		0.3 * (double) row[16] +
		0.2 * (double) row[17] +
		0.1 * (double) row[18] +
		0.3 * (double) row[19] +
		0.3 * (double) row[20] +
		0.2 * (double) row[21] +
		0.1 * (double) row[22] +
		0.3 * (double) row[23] +
		0.3 * (double) row[24] +
		0.2 * (double) row[25] +
		0.1 * (double) row[26] +
		0.3 * (double) row[27] +
		0.3 * (double) row[28] +
		0.2 * (double) row[29] +
		0.1 * (double) row[30] +
		0.3 * (double) row[31];
}

double f(int *row) {
	double score = 0.0;
	for (int i = 0; i < weights_length; ++i) {
		score += weights[i] * (double) row[i];
	}
	return score;
}

#include "assert.cpp"

int inline getKey(int **accessors, int attr, int index) {
	return accessors[attr][index];
}

int inline getValue(int **table, int **accessors, int index, int attr) {
	return table[getKey(accessors, attr, index)][attr];
}

class Comparator {
	public:
		int **table;
		int attr;
		Comparator(int **table, int attr) : table(table), attr(attr) {}
		bool operator() (int a, int b) {
			return table[a][attr] > table[b][attr];
		}
};

int bin_search(double needle, score_t haystack[], int start, int length) {
	if (length == 0) { return 0; }
	int pos;
	do {
		pos = start + length / 2;
		if (needle == haystack[pos].value) {
			return pos + 1;
		}
		else if (needle > haystack[pos].value) {
			if (length == 1) {
				return pos;
			}
			else {
				length = pos - start;
			}
		}
		else if (needle < haystack[pos].value) {
			if (length == 1) {
				return pos + 1;
			}
			else {
				length = start + length - pos;
				start = pos;
			}
		}
		else {
			assert(false && "dette kan ikke skje");
		}
	} while(true);
}

int seq_search(double needle, score_t haystack[], int start, int length) {
	int pos;
	for (pos = 0; pos < length; ++pos) {
		if (needle >= haystack[pos].value) {
			break;
		}
	}
	return pos;
}

void result_add_to_pos(int key, double score, int pos, result_t &result) {
	int movecount;

	// Expand result size
	if (result.size < result.capacity) {
		__sync_fetch_and_add(&result.size, 1);
	}

	// Move elements to make room
 	movecount = result.size - pos - 1;
	if (movecount > 0) {
		memmove(
			&result.rows[pos+1],		// destination
			&result.rows[pos], 			// source
			movecount * sizeof(score_t) // number of bytes
		);
	}

	// Insert new element
	if (pos < result.capacity) {
		result.rows[pos].key = key;
		result.rows[pos].value = score;
	}

#if DEBUG > 2
	printf("RESULT: ");
	printf("(%d,%f)", result.rows[0].key, result.rows[0].value);
	for (int i = 1; i < result.size; ++i) {
		printf(", (%d,%f)", result.rows[i].key, result.rows[i].value);
	}
	printf("\n");
#endif
}

void result_seq_add(int key, double score, result_t &result) {
	if (result.size == result.capacity && 
		score <= result.rows[result.size-1].value) {
		return;
	}
	int pos = seq_search(score, result.rows, 0, result.size);
	result_add_to_pos(key, score, pos, result);
}

void result_bin_add(int key, double score, result_t &result) {
	if (result.size == result.capacity && 
		score <= result.rows[result.size-1].value) {
		return;
	}
	int pos = bin_search(score, result.rows, 0, result.size);
	result_add_to_pos(key, score, pos, result);
}

void ta(
	int **table, int tableLength, 
	int **accessors,  int accessorsLength,
	double (*f)(int *row),
	int k, 
	void (*add_result)(int,double,result_t&),
	result_t &result) 
{
	double score, threshold;
	int key;
	int *best = new int[accessorsLength];
	for (int j = 0; j < tableLength; ++j) {
		for (int i = 0; i < accessorsLength; ++i) {
			key = getKey(accessors, i, j);
			if (table[key][accessorsLength] == 0) {
				table[key][accessorsLength] = 1; // Seen
				score = f(table[key]);
				add_result(key, score, result);
			}
			best[i] = table[key][i];
		}
		threshold = f(best);
		if (result.size >= k && result.rows[result.size-1].value >= threshold) {
			break;
		}
	}
	delete[] best;
}

#include "hipta.cpp"

/*
 * Testing
 */

void initTable(int** table, int tableLength, int attrLen) {
	for (int i = 0; i < tableLength; ++i) { table[i][attrLen] = 0; }
}

int count_seen(int **table, int length, int attrcount) {
	int count = 0;
	for (int i = 0; i < length; ++i) {
		if (table[i][attrcount])
			++count;
	}
	return count;
}

void table_close(int **table, const int m) {
	for (int i = 0; i < m; ++i) {
		delete[] table[i];
	}
	delete[] table;
}

int **table_open(const char* file, const int m, const int n) {
	int** table = new int*[m];
	for (int i = 0; i < m; ++i) {
		table[i] = new int[n+1];
		table[i][n] = 0;
	}
	table_read(file, table, m, n);
	return table;
}

void table_reset(int** table, int m, int n) {
	for (int i = 0; i < m; ++i) { table[i][n] = 0; }
}

int **table_accessor_open(int **table, const int m, const int n) {

	// Allocate data
	int **accessor = new int*[n];
	for (int i = 0; i < n; ++i) {
		accessor[i] = new int[m];
	}

	// Generate sorted accessors for each column
	for (int i = 0; i < n; ++i) {
		Comparator cmp(table, i);
		accessor[i][0] = m; // length
		for (int j = 0; j < m; j++) {
			accessor[i][j] = j;
		}
		std::sort(&accessor[i][0], &accessor[i][m], cmp);
	}

	return accessor;
}

void table_accessor_close(int **accessor, const int n) {
	for (int i = 0; i < n; ++i) {
		delete[] accessor[i];
	}
	delete[] accessor;
}

int partsize(int k, int t, int pmin, int pmax) {
	int partsize = k / t;
	if (partsize < pmin) {
		partsize = pmin;
	}
	else if (partsize > pmax) {
		partsize = pmax;
	}
	return partsize;
}

class Test {
	public:
		static const int ATTRCOUNT = 7;
		enum { K, N, M, D, T, PMIN, PMAX };

		Test(int attribute, const char* colname, bool quick = false, 
			const char *inputfile = "../data/topk/256x32.tbl") 
			: attribute(attribute), colname(colname), quick(quick),
		inputfile(inputfile) { 
			for (int i = 0; i < ATTRCOUNT; ++i) {
				attributes[i] = new int;
				attrcount[i] = 0;
			}
		}

		~Test() {
			for (int i = 0; i < ATTRCOUNT; ++i) {
				delete attributes[i];
			}
		}

		void set(int attr, int argc, ...) {
			delete[] attributes[attr];
			attributes[attr] = new int[argc];
			attrcount[attr] = argc;
			va_list argv;
			va_start(argv, argc);
			for (int i = 0; i < argc; ++i) {
				attributes[attr][i] = va_arg(argv, int);
			}
			va_end(argv);
		}

		int get(int attr, int index) {
			return attrcount[attr] > index ? 
				attributes[attr][index] :
				attributes[attr][attrcount[attr]-1];
		}

		void run() {
			// Input relation
			int **input;
			int **sorted_input;

			// Output relation
			result_t result;
			result_t result_expected;

			// Parallel params
			ThreadPool *threadpool;
			int p;

			// Run algorithms

			if (quick) {
				input = table_open(inputfile, get(M,0), get(N,0));
				sorted_input = table_accessor_open(input, get(M,0), get(N,0));
			}

			printf("# run\t%s\ttime\tseen\n", colname);
			for (int i = 0; i < attrcount[attribute]; ++i) {

				int k = get(K, i);
				int n = get(N, i);
				int m = get(M, i);
				int pmin = get(PMIN, i);
				int pmax = get(PMAX, i);
				double (*scoring_func)(int *row);
				int testid = 1;
				
				// Scoring function
				// TODO.. a bit more generic maybe? and globals..
				weights_length = n;
				switch (n) {
					case 2:
						scoring_func = f_2;
						break;
					case 4:
						scoring_func = f_4;
						break;
					case 8:
						scoring_func = f_8;
						break;
					case 16:
						scoring_func = f_16;
						break;
					case 32:
						scoring_func = f_32;
						break;
					default:
						scoring_func = f;
						break;
				};

				// Read input
				if (!quick) {
					input = table_open(inputfile, m, n);
					sorted_input = table_accessor_open(input, m, n);
				}

				// Generate solution
				result_init(k, result_expected);
				ta(input, m, sorted_input, n, 
					scoring_func, k, result_seq_add, result_expected);
				table_reset(input, m, n);

				// Sequential runs
				printf("#sequential\n");
				for (int j = 0; j < 10; ++j) {
					result_init(k, result);
					tic();
					ta(input, m, sorted_input, n, 
						scoring_func, k, result_bin_add, result);
					printf("%d\t%d\t%f\t%d\n", testid,
						get(attribute, i), toc(), 
						count_seen(input, m, n));
					assertResult(result_expected, result);
					table_reset(input, m, n);
					result_destroy(result);
				}
				++testid;

				// Parallel runs
				int threadcounts = (attribute == T) ? 
					1 : attrcount[T];
				for (int ti = 0; ti < threadcounts; ++ti) {
					int t = get(T, (attribute == T) ? i : ti);
					printf("#parallel %d\n", t);
					for (int j = 0; j < 10; ++j) {
						result_init(k, result);
						p = partsize(k, t, pmin, pmax);
						hipta_init(input, m, 
							sorted_input, n, scoring_func, k, p, result);
						threadpool = new ThreadPool(t, hipta_worker);
						tic();
						hipta_threadpool(threadpool);
						printf("%d\t%d\t%f\t%d\n", testid,
							get(attribute, i), toc(), 
							count_seen(input, m, n));
						assertResult(result_expected, result);
						delete threadpool;
						result_destroy(result);
						hipta_free();
						table_reset(input, m, n);
					}
					++testid;
				}

				// Cleanup
				result_destroy(result_expected);

				if (!quick) {
					table_accessor_close(sorted_input, n);
					table_close(input, m);
				}
			}

			if (quick) {
				table_accessor_close(sorted_input, get(N,0));
				table_close(input, get(M,0));
			}
		}

	private:
		int attribute;
		int *attributes[ATTRCOUNT];
		int attrcount[ATTRCOUNT];
		const char* colname;
		bool quick;
		const char* inputfile;
};

void test0() {
	printf("#Test 0\n");
	Test test(Test::K, "k");
	test.set(Test::K, 4, 5, 10, 50, 100);
	test.set(Test::N, 1, 4);
	test.set(Test::M, 1, 1600);
	test.set(Test::T, 4, 1, 2, 12, 24, 1024);
	test.set(Test::PMIN, 1, 50);
	test.set(Test::PMAX, 1, 500);
	test.run();
}

void test1() {
	printf("#Test 1\n");
	Test test(Test::K, "k", true);
	test.set(Test::K, 7, 5, 10, 50, 100, 1000, 5000, 10000);
	test.set(Test::N, 1, 4);
	test.set(Test::M, 1, 128000000);
	test.set(Test::T, 5, 1, 2, 12, 24, 1024);
	test.set(Test::PMIN, 1, 50);
	test.set(Test::PMAX, 1, 500);
	test.run();
}

void test2() {
	printf("#Test 2\n");
	Test test(Test::N, "n");
	test.set(Test::K, 1, 1000);
	test.set(Test::N, 4, 4, 8, 16, 32);
	test.set(Test::M, 1, 16000000);
	test.set(Test::T, 5, 1, 2, 12, 24, 1024);
	test.set(Test::PMIN, 1, 200);
	test.set(Test::PMAX, 1, 400);
	test.run();
}

void test3() {
	printf("#Test 3\n");
	Test test(Test::M, "m");
	test.set(Test::K, 1, 1000);
	test.set(Test::N, 1, 4);
	test.set(Test::M, 6, 4000000, 16000000, 32000000, 
		64000000, 128000000, 256000000);
	test.set(Test::T, 5, 1, 2, 12, 24, 1024);
	test.set(Test::PMIN, 1, 200);
	test.set(Test::PMAX, 1, 400);
	test.run();
}
void test4() {
	printf("#Test 4\n");
	Test test(Test::PMIN, "pmin", true);
	test.set(Test::K, 1, 1000);
	test.set(Test::N, 1, 4);
	test.set(Test::M, 1, 128000000);
	test.set(Test::T, 5, 1, 2, 12, 24, 1024);
	test.set(Test::PMIN, 10, 1, 5, 10, 50, 100, 200, 
		300, 400, 500, 1000);
	test.set(Test::PMAX, 1, 1000);
	test.run();
}

void test5() {
	printf("#Test 5\n");
	Test test(Test::PMAX, "pmax", true);
	test.set(Test::K, 1, 1000);
	test.set(Test::N, 1, 4);
	test.set(Test::M, 1, 128000000);
	test.set(Test::T, 5, 1, 2, 12, 24, 1024);
	test.set(Test::PMIN, 1, 1);
	test.set(Test::PMAX, 10, 1, 5, 10, 50, 100, 200, 
		300, 400, 500, 1000);
	test.run();
}

void test6(int skew = 0) {
	Test *test;
	if (skew == 1) {
		printf("#Test 6 skew\n");
		test = new Test(Test::T, "t", true, "../data/topk/256x32skew.tbl");
		test->set(Test::N, 1, 8);
	}
	else if (skew == 2) {
		printf("#Test 6 skew (negative corr)\n");
		test = new Test(Test::T, "t", true, "../data/topk/256x2acorr.tbl");
		test->set(Test::N, 1, 2);
	}
	else {
		printf("#Test 6\n");
		test = new Test(Test::T, "t", true);
		test->set(Test::N, 1, 8);
	}
	test->set(Test::K, 1, 1000);
	test->set(Test::N, 1, 8);
	test->set(Test::M, 1, 256000000);
	test->set(Test::T, 13, 1, 2, 4, 6, 8, 12, 24, 
		32, 64, 128, 256, 512, 1024);
	test->set(Test::PMIN, 1, 200);
	test->set(Test::PMAX, 1, 400);
	test->run();
}

int main(int argc, char* argv[]) {
	setbuf(stdout, NULL);

	int test = (argc > 1) ? atoi(argv[1]) : 1;

	switch(test) {
		case 0:
			test0();
			break;
		case 1:
			test1();
			break;
		case 2:
			test2();
			break;
		case 3:
			test3();
			break;
		case 4:
			test4();
			break;
		case 5:
			test5();
			break;
		case 6:
			test6();
			break;
		case 7:
			test6(1);
			break;
		case 8:
			test6(2);
			break;
		default:
			test1();
			test2();
			test3();
			test4();
			test5();
			test6();
			break;
	}
}
