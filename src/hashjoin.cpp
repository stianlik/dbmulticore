#include "util.cpp"

#ifndef JOINVAR_LEFT
#define JOINVAR_LEFT key
#endif

#ifndef JOINVAR_RIGHT
#define JOINVAR_RIGHT key
#endif

#ifndef JOINOP
#define JOINOP ==
#endif

using namespace std;

void hashjoin(
	row16_t *left, int leftLength, 
	row16_t *right, int rightLength,  
	row32_t *result, int &resultLength
) {
	HashMap hashMap(leftLength);
	bucket_t *bucket;
	int resultCount = 0;

	for (int i = 0; i < leftLength; ++i) {
		hashMap.put(left[i]);
	}

	for (int i = 0; i < rightLength; ++i) {
		bucket = hashMap.getBucket(right[i].JOINVAR_RIGHT);
		do {
			for (int j = 0; j < bucket->itemCount; ++j) {
				if (right[i].JOINVAR_RIGHT JOINOP bucket->items[j].key) {
					row32_t pair = {
						static_cast<row16_t>(bucket->items[j]),
						static_cast<row16_t>(right[i])
					};
					result[resultCount++] = pair;
				}
			}
		} while ( (bucket = bucket->overflow) != NULL);
	}
	resultLength = resultCount;
}
