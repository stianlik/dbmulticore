CC=g++
CFLAGS=-Wall -lm -std=c++0x -pedantic # -DDEBUG
CFLAGS+=-fopenmp
CFLAGS+=-lpthread
#CFLAGS+=-g
CFLAGS+=-O2

all: test

test: testrunner.cpp hashjoin.cpp util.cpp PerfCounter.cpp hashjoin_omp.cpp select.cpp
	$(CC) $(CFLAGS) -o test testrunner.cpp

ta: ta.cpp hipta.cpp threadpool.cpp read_table.cpp assert.cpp
	#$(CC) $(CFLAGS) -c -o ta.o ta.cpp
	$(CC) $(CFLAGS) -o ta ta.cpp

clean:
	rm *.o
