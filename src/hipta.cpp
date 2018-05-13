#define SEEN hipta_data.accessorsLength

/**
 * This file includes a parallel implementation of TA,
 * using the HIPTA algorithm. Data strucutres for shared memory
 * access is also included. The algorithm used for the report is
 * implemented with function name hipta_threadpool.
 */

struct hipta_node_t {
	score_t score;
	hipta_node_t *next;
	pthread_mutex_t lock;
};

struct hipta_t {
	int **table; 
	int tableLength;
	int **accessors;
	int accessorsLength;
	double (*f)(int *row);
	int k;
	result_t *result;
	int *best;
	int lastrow;
	pthread_mutex_t resultlock;
	int partsize;
	hipta_node_t *head;
	double threshold;
	double worst;
};

hipta_t hipta_data; // Shared memory

void hipta_addResultList(int key, double score) {
	hipta_node_t *node = new hipta_node_t;
	pthread_mutex_init(&node->lock, NULL);
	node->score.key = key;
	node->score.value = score;
	pthread_mutex_lock(&node->lock);

	hipta_node_t *prev = hipta_data.head;
	hipta_node_t *next = prev->next;
	while (true) {
		while (next->score.value > node->score.value) {
			prev = next;
			next = next->next;
		}
		pthread_mutex_lock(&prev->lock);
		//pthread_mutex_lock(&next->lock);
		if (next->score.value <= node->score.value) {
			node->next = next;
			prev->next = node;
			//pthread_mutex_unlock(&next->lock);
			pthread_mutex_unlock(&node->lock);
			pthread_mutex_unlock(&prev->lock);
			return;
		}
		else {
			prev = next;
			next = prev->next;
			pthread_mutex_unlock(&next->lock);
			pthread_mutex_unlock(&prev->lock);
		}
	}
}

void inline hipta_moveResultToArray() {
	hipta_node_t *node = hipta_data.head;
	hipta_data.result->size = 0;
	for (int i = 0; i < hipta_data.k && node->next->score.key != -1; ++i) {
		node = node->next;
		hipta_data.result->rows[i] = node->score;
		++hipta_data.result->size;
	}
	node->next->score.key = -1;
	node->next->score.value = -1.0;
}

void hipta_result_add(int key, double score, result_t &result) {
	if (result.size == result.capacity && 
		score <= hipta_data.worst) {
		return;
	}
	pthread_mutex_lock(&hipta_data.resultlock);
	int pos = bin_search(score, result.rows, 0, result.size);
	result_add_to_pos(key, score, pos, result);
	hipta_data.worst = result.rows[result.size-1].value;
	pthread_mutex_unlock(&hipta_data.resultlock);
}

void *hipta_processRow(void *row) {
	int key, seen;
	double score;
	long first = (long)row;
    int last = first + hipta_data.partsize;
    if (last > hipta_data.lastrow) { 
    	last = hipta_data.lastrow;
    }
	for (; first <= last; ++first) {
		for (int i = 0; i < hipta_data.accessorsLength; ++i) {
			key = getKey(hipta_data.accessors, i, first);
			seen = __sync_fetch_and_add(&hipta_data.table[key][SEEN], 1);
			if (seen == 0) {
				score = hipta_data.f(hipta_data.table[key]);
				//hipta_addResultList(key, score);
				hipta_result_add(key, score, *hipta_data.result);
			}
		}
	}

	// Last thread responsible for threshold value
	if (hipta_data.lastrow == last) {
		int* best = new int[SEEN]; // TODO why did this work, and not the global?
		for (int i = 0; i < hipta_data.accessorsLength; ++i) {
			key = getKey(hipta_data.accessors, i, last);
			//hipta_data.best[i] = hipta_data.table[key][i];
			best[i] = hipta_data.table[key][i];
		}
		//hipta_data.threshold = hipta_data.f(hipta_data.best);
		hipta_data.threshold = hipta_data.f(best);
		delete[] best;
	}

	return NULL;
}

void inline hipta_calcLastRow(int &j, int &chunksize) {
	hipta_data.lastrow = j+chunksize-1; 
	if (hipta_data.lastrow >= hipta_data.tableLength) {
		hipta_data.lastrow = hipta_data.tableLength-1;
	}
}

void hipta_simple(const int threadcount) {
	pthread_t *threadhandles = new pthread_t[threadcount];
	int chunksize = threadcount * hipta_data.partsize;
	int t = 0;

	for (int j = 0; j < hipta_data.tableLength; j += threadcount) {

		// Process partsize rows per thread
		hipta_calcLastRow(j, chunksize);
		t = 0;
		for (long row = j; row <= hipta_data.lastrow; row+=hipta_data.partsize) {
			pthread_create(&threadhandles[t++], NULL, hipta_processRow, (void*) row);
		}

		// Sync
		for (t = 0; t < threadcount; t++) {
			pthread_join(threadhandles[t], NULL);
		}

		// Threshold (termination)
		if (hipta_data.result->size >= hipta_data.k && 
			hipta_data.result->rows[hipta_data.result->size-1].value >= hipta_data.threshold) {
			break;
		}
	}

	delete[] threadhandles;
}

void copy() {
	hipta_node_t *node = hipta_data.head;
	int i;
	for (i = 0; i < hipta_data.k && node->next->score.key != -1; ++i) {
		node = node->next;
		hipta_data.result->rows[i] = node->score;
	}
}

void hipta_threadpool(ThreadPool *threadPool) {
	int chunksize = threadPool->threadcount * hipta_data.partsize;
	double worst;
	for (int j = 0; j < hipta_data.tableLength; j += chunksize) {

		// Process partsize rows per thread
		hipta_calcLastRow(j, chunksize);
		for (long row = j; row <= hipta_data.lastrow; row+=hipta_data.partsize) {
			threadPool->push((void*) row);
		}
		threadPool->sync();

		// Threshold (termination)
		//hipta_moveResultToArray();
		worst = hipta_data.result->rows[hipta_data.result->size-1].value;
		if (hipta_data.result->size >= hipta_data.k && worst >= hipta_data.threshold) {
			return;
		}
	}
}

void hipta_init(
	int **table, int tableLength, 
	int **accessors,  int accessorsLength,
	double (*f)(int *row),
	int k, int partsize, result_t &result
) 
{
	// Shared data
	hipta_data = {
		table, tableLength, accessors, accessorsLength, f, k, &result, 
		new int[hipta_data.accessorsLength], 1
	};
	hipta_data.partsize = partsize;
	hipta_data.head = new hipta_node_t;
	hipta_data.threshold = 0.0;
	hipta_data.worst = 0.0;

	// Linked list
	hipta_data.head->score = {0, 0.0};
	pthread_mutex_init(&hipta_data.head->lock, NULL);
	hipta_data.head->next = new hipta_node_t;
	hipta_data.head->next->score = {-1, -1.0};
	hipta_data.head->next->next = NULL;
	pthread_mutex_init(&hipta_data.head->lock, NULL);

	// Initialize locks
	pthread_mutex_init(&hipta_data.resultlock, NULL);
}

void hipta_free() {
	hipta_node_t *cur = hipta_data.head;
	hipta_node_t *next;
	while (cur != NULL) {
		next = cur->next;
		pthread_mutex_destroy(&cur->lock);
		delete cur;
		cur = next;
	}
	pthread_mutex_destroy(&hipta_data.resultlock);
	delete[] hipta_data.best;
}

void * hipta_worker(void *threadPool) {
    ThreadPool *tp = static_cast<ThreadPool*>(threadPool);
    int **row = new int*;
    while (tp->pop((void**) row)) {
		hipta_processRow( *row );
		tp->taskDone();
    }
    delete row;
	//printf("worker done\n");
	tp->workerDone();
    return NULL;
}
