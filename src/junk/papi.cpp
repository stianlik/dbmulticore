#include <stdlib.h>
#include <stdio.h>
#include <papi.h>
#include <iostream>
#define NUM_EVENTS 3

using namespace std;

void handle_error (int retval)
{
     printf("PAPI error %d: %s\n", retval, PAPI_strerror(retval));
     exit(1);
}

int main(int argc, char* argv[]) {

	int err = 0;
	
	// Start counters
	int counters[] = {
		PAPI_L1_TCM,
		PAPI_L2_TCM,
		PAPI_L3_TCM,
		//PAPI_TLB_DM,
		//PAPI_TLB_IM,
		//PAPI_BR_MSP,
		//PAPI_TOT_INS,
		//PAPI_TOT_CYC
	};
	if ( (err = PAPI_start_counters(counters, NUM_EVENTS)) != PAPI_OK) {
		handle_error(err);
	}

	// Calculate
	cout << "test" << endl;
	int test[1000000];
	for (int i = 1; i < 100000000; i++) {
		test[rand()%1000000] = i;
	}

	// Read counters
	long long values[NUM_EVENTS];
	if ( (err = PAPI_read_counters(values, NUM_EVENTS)) != PAPI_OK) {
		handle_error(err);
	}

	// Stop counters
	if ( (err = PAPI_stop_counters(values, NUM_EVENTS)) != PAPI_OK) {
		handle_error(err);
	}

	// Print result
	cout << "Level 1 cache misses:\t" << values[0] << endl;
	cout << "Level 2 cache misses:\t" << values[1] << endl;
	cout << "Level 3 cache misses:\t" << values[2] << endl;
	cout << "Data TLB misses:\t" << values[3] << endl;
	cout << "Instruction TLB misses:\t" << values[4] << endl;
	cout << "Branch mispredictions:\t" << values[5] << endl;
	cout << "Instructions completed:\t" << values[6] << endl;
	cout << "Total cycles:\t\t" << values[7] << endl;
}
