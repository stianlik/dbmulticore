#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <string.h>

#include "lib/lib.h"

void hello();

int main(int argc, char* argv[]) {

	// Thread count from command line arguments
	int thread_count = strtol(argv[1], NULL, 10);

	# pragma omp parallel num_threads(thread_count)
	hello();

	return 0;
}

void hello() {
	int my_rank = omp_get_thread_num();
	int thread_count = omp_get_num_threads();
	int cpu = sched_getcpu();
	printf("Hello from thread %d of %d, running on %d\n", 
		my_rank, thread_count, cpu);
}
