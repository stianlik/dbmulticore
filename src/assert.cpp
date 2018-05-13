#include <cassert>
void assertResult(result_t &expected, result_t &actual) {

	if (expected.size != actual.size) {
		printf("Expected size of %d but was %d\n", 
		expected.size, actual.size);
		exit(1);
	}

	for (int i = 0; i < expected.size; ++i) {
		if (expected.rows[i].value != actual.rows[i].value) {
			printf(
				"Expected value %f but was %f in row %d, size was %d\n", 
				expected.rows[i].value, actual.rows[i].value, i, actual.size
			);

			for (int i = 0; i < expected.size; ++i) {
				for (int j = i+1; j < expected.size; ++j) {
					if (actual.rows[i].key == actual.rows[j].key) {
						printf("Duplicate key found row %d and row %d\n", i, j);
						break;
					}
				}
			}

			exit(1);
		}
	}
}

