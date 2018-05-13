#include<ctime>

class Perfcounter {
	public:
		Perfcounter();
		void start();
		void checkpoint();
		double startTime;
		double endTime;
		double totalTime;
};

Perfcounter::Perfcounter() { }

void Perfcounter::start() {
	cout << "# " << "Thread count\t" << "Time\t" << endl;
	totalTime = 0;
	startTime = omp_get_wtime();
}

void Perfcounter::checkpoint() {
	endTime = omp_get_wtime();
	int time = 1000 * (endTime - startTime);
	totalTime += time;
	printf("%d\t%d\t\n", omp_get_max_threads(), time);
	startTime = omp_get_wtime();
}
