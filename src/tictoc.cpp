#include "wtime.c"
double startTime[100];
int curTic = -1;

void tic() {
	startTime[++curTic] = wtime();
}

double toc() {
	return wtime() - startTime[curTic--];
}
