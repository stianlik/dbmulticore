#include <omp.h>
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

void hashjoin_omp(
	row16_t *left, int leftLength, 
	row16_t *right, int rightLength,  
	row32_t *result, int &resultLength
) {
	HashMapOmp hashMap(leftLength);
	int threadcount = omp_get_max_threads();
	vector<row32_t> *partitions = new vector<row32_t>[threadcount];
	int *offsets = new int[threadcount];
	int chunksize = rightLength/threadcount;

	// Build
	#pragma omp parallel for default(none) firstprivate(leftLength) shared(left,hashMap)
	for (int i = 0; i < leftLength; ++i) {
		hashMap.put(left[i]); // Bucket locked
	}

	// Probe
	//#pragma omp parallel default(none) firstprivate(chunksize,rightLength) shared(hashMap,partitions,right)
	#pragma omp parallel default(none) firstprivate(chunksize,rightLength,threadcount) shared(hashMap,partitions,right,result,offsets)
	{
		int my_rank = omp_get_thread_num();
		partitions[my_rank].reserve(chunksize);
		bucket_t *bucket;

		#pragma omp for schedule(static, chunksize)
		for (int i = 0; i < rightLength; ++i) {
			bucket = hashMap.getBucket(right[i].JOINVAR_RIGHT);
			do {
				for (int j = 0; j < bucket->itemCount; ++j) {
					if (right[i].JOINVAR_RIGHT JOINOP bucket->items[j].key) {
						row32_t pair = {
							static_cast<row16_t>(bucket->items[j]),
							static_cast<row16_t>(right[i])
						};
						partitions[my_rank].push_back(pair);
					}
				}
			} while ( (bucket = bucket->overflow) != NULL);
		}
	//}
//	
//	#pragma omp parallel default(none) firstprivate(threadcount, chunksize) shared(partitions,result,offsets)
//	{
		// Calculate offsets (prefix sum)
		#pragma omp for schedule(static, 1)
		for (int i = 0; i < threadcount; ++i) {
			offsets[i] = 0;
			for (int j = 0; j < i; ++j) {
				offsets[i] += partitions[j].size();
			}
		}

		// Combine results
		int first;
		#pragma omp for schedule(static, chunksize)
		for (int i = 0; i < threadcount; ++i) {
		 	first = offsets[i];
			for (int j = 0; j < (int) partitions[i].size(); j++) {
				result[first + j] = partitions[i][j];
			}
		}
	}

	resultLength = offsets[threadcount-1] + partitions[threadcount-1].size();

	delete[] partitions;
	delete[] offsets;
}
