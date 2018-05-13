#include <queue>

/**
 * Busy waiting thread pool. Used mutexes and conditionals
 * for the first version, and this would be a more appropriate
 * method for production code. However, busy waiting in combination
 * with atomic operations did result in a performance gain.
 */
class ThreadPool {

	bool run;
	pthread_t *threadhandles; 
	std::queue<void*> queue;
	pthread_mutex_t queue_lock;
	pthread_cond_t queue_cond; 

	// Busy-wait synchronization
	volatile int activecount;
	volatile int taskcount;

	public: 
		int threadcount;

		ThreadPool(int threadcount, void*(*_worker)(void*)) : 
			run(true), taskcount(0), threadcount(threadcount)
		{
			queue_lock = PTHREAD_MUTEX_INITIALIZER;
			queue_cond = PTHREAD_COND_INITIALIZER; 
			threadhandles = new pthread_t[threadcount];
			activecount = threadcount;
			for (long t = 0; t < threadcount; t++) {
				pthread_create( &threadhandles[t], NULL, _worker, (void*) this);
			}
		}

		~ThreadPool() {

			// Terminate signal
			run = false;
			pthread_cond_broadcast(&queue_cond);

			// Wait for threads to terminate
			while (activecount > 0) { pthread_cond_broadcast(&queue_cond); }

			// Free threads
			for (long t = 0; t < threadcount; t++) {
				pthread_join(threadhandles[t], NULL);
			}
			delete[] threadhandles;

			// Free locks
			pthread_mutex_destroy(&queue_lock);
			pthread_cond_destroy(&queue_cond);
		}

		void push(void *value) {
			__sync_fetch_and_add(&taskcount, 1);
    		pthread_mutex_lock(&queue_lock);
			queue.push(value);
    		pthread_cond_signal(&queue_cond);
    		pthread_mutex_unlock(&queue_lock);
		}

		bool pop(void **value) {

			// Wait for new tasks
    		pthread_mutex_lock(&queue_lock);
    		while (queue.empty()) {
				if (run) {
					pthread_cond_wait(&queue_cond, &queue_lock);
				}
				else {
    				pthread_mutex_unlock(&queue_lock);
    				return false;
    			}
			}

			// Return new task in **value
			*value = queue.front();
			queue.pop();
    		pthread_mutex_unlock(&queue_lock);

    		return true;
		}

		void workerDone() {
			__sync_fetch_and_sub(&activecount, 1);
		}

		void taskDone() {
			__sync_fetch_and_sub(&taskcount, 1);
    	}
    	
		void sync() {
    		while (taskcount > 0) { /* wait */ }
		}
};
