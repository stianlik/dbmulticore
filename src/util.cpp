#ifndef UTIL_GUARD
#define UTIL_GUARD
#include<string>
#include<iostream>
#include<fstream>
#include<vector>
#include<cstdlib>
#include<sstream>
#include<cassert>
#include<iterator>
#include<unordered_map>
#include<ctime>
#include<map>
#include<cassert>
#include<typeinfo>
#include<omp.h>

#ifndef BUCKET_SIZE
#define BUCKET_SIZE 10000
#endif

#ifndef BUCKET_FACTOR
#define BUCKET_FACTOR 1.25
#endif

using namespace std;

struct row16_t {
	int key;
	int payload;
};

struct row32_t {
	row16_t left;
	row16_t right;
};

struct bucket_t {
	row16_t items[BUCKET_SIZE];
	int itemCount;
	bucket_t* overflow;
};

class HashMap {
	public:
		HashMap(int bucketCount);
		~HashMap();
		void freeOverflow(bucket_t *bucket);
		void put(row16_t &tuple);
		bucket_t *getBucket(int key);
		int hash(int key);
		int bucketCount;
		bucket_t *buckets;
};

class HashMapOmp : public HashMap {
	public:
		HashMapOmp(int bucketCount);
		~HashMapOmp();
		void put(row16_t &tuple);
		omp_lock_t *locks;
};

class TupleTokenizer {
	public:
		TupleTokenizer(string line);
		template<class T> void next(T &target);
	private:
		string line;
		int startPos;
		int nextPos;
};

/**
 * TupleTokenizer.c
 */

TupleTokenizer::TupleTokenizer(string line) : line(line), startPos(0), nextPos(-1) { }

template<class T>
void TupleTokenizer::next(T &target) {
	startPos = nextPos+1;
	nextPos = line.find('|', startPos);
	stringstream ss(line.substr(startPos, nextPos - startPos));
	ss >> target;
}

template<>
void TupleTokenizer::next(string &target) {
	startPos = nextPos+1;
	nextPos = line.find('|', startPos);
	stringstream ss(line.substr(startPos, nextPos - startPos));
	target = ss.str();
}

/**
 * HashMap.cpp
 */

HashMap::HashMap(int elementCount) {
	bucketCount = (elementCount  * BUCKET_FACTOR) / BUCKET_SIZE;

	buckets = new bucket_t[bucketCount];
	for (int i = 0; i < bucketCount; ++i) {
		buckets[i] = {};
	}
}

HashMap::~HashMap() {
	for (int i = 0; i < bucketCount; i++) {
		freeOverflow(&buckets[i]);
	}
	delete[] buckets;
}

void HashMap::freeOverflow(bucket_t *bucket) {
	if (bucket->overflow != NULL) {
		freeOverflow(bucket->overflow);
		delete bucket->overflow;
	}
}

int HashMap::hash(int key) {
	return key % bucketCount;
}

void HashMap::put(row16_t &tuple) {
	int h = hash(tuple.key);
	bucket_t *bucket = &buckets[h];
	while (bucket->itemCount == BUCKET_SIZE) {
		if (bucket->overflow == NULL) {
			bucket->overflow = new bucket_t;
			bucket->overflow->overflow = NULL;
			bucket->overflow->itemCount = 0;
		}
		bucket = bucket->overflow;
	}
	bucket->items[bucket->itemCount] = tuple;
	bucket->itemCount++;
}

bucket_t *HashMap::getBucket(int key) {
	return &buckets[hash(key)];
}

/**
 * HashMapOmp.cpp
 */

HashMapOmp::HashMapOmp(int elementCount) : HashMap(elementCount) {
	locks = new omp_lock_t[bucketCount];
	for (int i = 0; i < bucketCount; ++i) {
		omp_init_lock(&locks[i]);
	}
}

void HashMapOmp::put(row16_t &tuple) {
	int h = hash(tuple.key);
	bucket_t *bucket = &buckets[h];
	omp_set_lock(&locks[h]);
	while (bucket->itemCount == BUCKET_SIZE) {
		if (bucket->overflow == NULL) {
			bucket->overflow = new bucket_t;
			bucket->overflow->overflow = NULL;
			bucket->overflow->itemCount = 0;
		}
		bucket = bucket->overflow;
	}
	bucket->items[bucket->itemCount] = tuple;
	bucket->itemCount++;
	omp_unset_lock(&locks[h]);
}

HashMapOmp::~HashMapOmp() {
	for (int i = 0; i < bucketCount; ++i) {
		omp_destroy_lock(&locks[i]);
	}
	delete[] locks;
}

/**
 * TableReader.c
 */

template<class T> T readRow(string line) {
	cout << "Reader not implemented for this row" << endl;
	exit(1);
	return nullptr;
}

template<> row16_t readRow(string line) {
	row16_t r;
	TupleTokenizer tokenizer(line);
	tokenizer.next(r.key);
	tokenizer.next(r.payload);
	return r;
}

template<class T>
void readTable(const char* filepath, vector<T> &table) {

	ifstream fin;
	fin.open(filepath);
	if (fin.fail()) {
		cout << "Error reading file " << filepath << endl;
		exit(1);
	}

	string line;
	while (getline(fin, line)) {
		table.push_back(readRow<T>(line));
	}

	fin.close();
}

template<class T>
void freeTable(vector<T*> table) {
	for (auto it = table.begin(); it != table.end(); it++) {
		delete *it;
	}
}
#endif
