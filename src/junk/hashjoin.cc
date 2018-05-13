//
// Hash Join
//

#include <memory>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <exception>
#include <vector>
#ifdef STLPORT
#include <hash_map>
#else
#include <ext/hash_map>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pthread.h>

#include <commons/check.h>
#include <commons/deque.h>
#include <commons/files.h>
#include <commons/hash.h>
#include <commons/region.h>
#include <commons/strings.h>
#include <commons/threads.h>
#include <commons/time.h>

#include <boost/scoped_array.hpp>

#ifdef NUMA
#ifdef TILE64
#include <linux/numa.h>
#else
#include <numa.h>
#endif
#endif

using namespace std;
using namespace boost;
#ifndef STLPORT
using namespace __gnu_cxx;
#endif
using namespace commons;

typedef hash_map<
  const char*,
  const void*,
  hash<const char*>,
  eqstr,
  region_alloc<int>
> my_hash_map;

class hmap : public my_hash_map
{
public:
  void
  resize(size_type hint)
  {
    // cout << "resizing " << this << " to " << hint << endl;
    my_hash_map::resize(hint);
  }
};

// TODO use dependency injection!
unsigned int ncpus             = 1;
const hmap::size_type map_size = 1000000; // 1MB
const size_t min_bucket_size   = 100000; // 100KB

/**
 * Buckets are produced in the hash-partitioning phase. These are simple
 * storage containers.
 */
class bucket
{
public:
  ~bucket()
  {
    for (size_t i = 0; i < bufs.size(); i++) {
      delete [] bufs[i];
    }
  }
  /**
   * The sizes of the bufs. Should always be the same length as bufs.
   */
  vector<size_t> sz;
  /**
   * The data that we hold.
   */
  vector<char*> bufs;
};

// TODO: typedef list< array<char, bucket_size> > bucket;

/**
 * An abstract in-memory database that holds "tuples" in a contiguous buffer.
 * The format/interpretation of the buffers is up to the subclasses.
 */
class db
{
public:
  db(const char *path) :
    buf(load_file(path, buflen, ncpus)),
    // TODO dependency injection
    bucket_size(max((size_t) min_bucket_size, buflen / ncpus / ncpus * 3))
  {
    scan(buf, buflen);
  }
  /**
   * Run hash-partitioning phase on all processors.
   */
  const bucket **partition();
  virtual ~db() { delete [] buf; }
  /**
   * Push a tuple into one of the buckets. Which bucket is determined by the
   * hash partitioning scheme.
   *
   * \param heads Array of "cursors" into each bucket.
   * \param bs Array of buckets.
   * \param s The string to hash.
   * \param p The start of the tuple.
   * \param nbytes The length of the tuple.
   */
  unsigned int push_bucket(vector<char *> & heads, bucket *bs, const char *s,
                           const char *p, size_t nbytes);
protected:
  /**
   * This routine runs on each processor to hash-partition the data into local
   * buckets.
   */
  virtual void partition1(unsigned int pid, bucket* bucket) = 0;
  char *buf;
  size_t buflen;
  size_t bucket_size;
};

/**
 * Movie database.
 */
class movdb : public db
{
public:
  movdb(const char *path) : db(path) {}
  virtual ~movdb() {}
  /**
   * Build the hash map in parallel.
   */
  const hmap *build(const bucket **movbucs);
protected:
  /**
   * Each node runs this routine to construct its local hash map.
   */
  void build1(unsigned int pid, const bucket **movbucs, hmap *h);
  void partition1(unsigned int pid, bucket* bucket);
};

/**
 * Actress database.
 */
class actdb : public db
{
public:
  actdb(const char *path) : db(path) {}
  virtual ~actdb() {}
  /**
   * Probe the hash maps with tuples from the actor buckets.
   */
  void probe(const hmap *hs, const bucket **actbucs, bool show_progress);
protected:
  /**
   * Each node runs this routine to probe into its local hash map using tuples
   * from actor buckets destined for that node.
   */
  void probe1(unsigned int pid, const hmap *ph, const bucket **actbucs);
  void partition1(unsigned int pid, bucket* bucket);
};

int
main(int argc, char *argv[])
{
  if (argc != 4) {
    fprintf(stderr, "hashjoin <ncpus> <movies> <actresses>\n");
    exit(1);
  }

  ncpus = atoi(argv[1]);
  const char *movies = argv[2];
  const char *actresses = argv[3];

  cout << "using " << ncpus << " cpus" << endl;

  // Load the data files.

  cout << "loading movies" << endl;
  timer tldmov("loading movies time: ");
  movdb mdb(movies);
  tldmov.print();

  cout << "loading actresses" << endl;
  timer tldact("loading actresses time: ");
  actdb adb(actresses);
  tldact.print();

  timer ttotal("total time: ");

  // Hash-partition the data among the nodes.

  cout << "hash-partitioning movies into per-core buckets" << endl;
  timer thpmov("hash-partitioning movies time: ");
  const bucket **movbucs = mdb.partition();
  thpmov.print();

  cout << "hash-partitioning actresses into per-core buckets" << endl;
  timer thpact("hash-partitioning actresses time: ");
  const bucket **actbucs = adb.partition();
  thpact.print();

  // Perform the hash-join.

  cout << "building with movies" << endl;
  timer tbuild("building with movies time: ");
  const hmap *hs = mdb.build(movbucs);
  tbuild.print();

  cout << "probing with actresses" << endl;
  timer tprobe("probing with actresses time: ");
  adb.probe(hs, actbucs, true);
  tprobe.print();

  ttotal.print();
  cout << "done" << endl;
}

const bucket **
db::partition()
{
  // Create the buckets.
  bucket **buckets = new bucket*[ncpus];
  for (unsigned int i = 0; i < ncpus; i++) {
    buckets[i] = new bucket[ncpus];
    check(buckets[i]);
    for (unsigned int j = 0; j < ncpus; j++) {
      // Each bucket should be twice as large as it would be given uniform
      // distribution. This is just an initial size; extending can happen.
      buckets[i][j].bufs.push_back(new char[bucket_size]);
      buckets[i][j].sz.push_back(0);
      check(buckets[i][j].bufs[0]);
    }
  }

  // Partition the data into the buckets using the hash function. Reason for
  // buckets: poor man's message passing. All the data from CPU i to CPU j goes
  // into bucket[i][j].
  vector<pthread_t> ts(ncpus);
  for (unsigned int i = 0; i < ncpus; i++) {
    ts[i] = method_thread(this, &db::partition1, i, buckets[i]);
  }
  for (unsigned int i = 0; i < ncpus; i++) {
    check(pthread_join(ts[i], NULL) == 0);
  }

  return const_cast<const bucket**>(buckets); // TODO why is this cast needed?
}

inline char *
next_tuple(char *p)
{
  char *next = unsafe_strstr(p, "\0\0", 0);
  return next == NULL ? p + strlen(p) : next + 2;
}

unsigned int
db::push_bucket(vector<char*> & heads, bucket *bs, const char *s, const char *p, size_t nbytes)
{
  size_t h = hash_djb2(s);
  unsigned int bucket = h % ncpus;
  if (heads[bucket] + nbytes < bs[bucket].bufs.back() + bucket_size) {
    memcpy(heads[bucket], p, nbytes);
    heads[bucket] += nbytes;
    return -1;
  } else {
    bs[bucket].sz.back() = heads[bucket] - bs[bucket].bufs.back();
    bs[bucket].bufs.push_back(new char[bucket_size]);
    check(bs[bucket].bufs.back());
    heads[bucket] = bs[bucket].bufs.back();
    return bucket;
  }
}

void
movdb::partition1(unsigned int pid, bucket *bs)
{
  // Calculate where our initial partition starts and ends (approximately -- if
  // we land in the middle of a tuple, we will advance to the next one first).
  char *partstart = pid == 0 ? buf : next_tuple(buf + buflen * pid / ncpus),
       *partend = pid == ncpus - 1 ?
                  buf + buflen - 1 :
                  next_tuple(buf + buflen * (pid + 1) / ncpus);
  cout << "cpu " << pid << " partition " <<
    partstart - buf << ".." << partend - buf << endl;
  // Position the writing heads at the head of each bucket. (TODO find better
  // name than head)
  vector<char*> heads(ncpus);
  for (unsigned int i = 0; i < ncpus; i++) {
    heads[i] = bs[i].bufs[0];
  }
  int counter = 0;
  char *p = partstart, *end = partend;
  // Iterate over the partitions.
  while (p < end) {
    char *title = p;
    char *release = strchr(p, '\0') + 1;
    p = strchr(release, '\0') + 2;
    // Copy this tuple into the correct local bucket.
    push_bucket(heads, bs, title, title, p - title);
    counter++;
  }
  // Record the written size of each bucket.
  for (unsigned int i = 0; i < ncpus; i++) {
    bs[i].sz.back() = heads[i] - bs[i].bufs.back();
  }
  cout << "movie count " << counter << " nbytes " << bs[0].sz.back()<< endl;
}

void
actdb::partition1(unsigned int pid, bucket *bs)
{
  // Calculate where our initial partition starts and ends (approximately -- if
  // we land in the middle of a tuple, we will advance to the next one first).
  char *partstart = pid == 0 ? buf : next_tuple(buf + buflen * pid / ncpus),
       *partend = pid == ncpus - 1 ?
                  buf + buflen - 1 :
                  next_tuple(buf + buflen * (pid + 1) / ncpus);
  cout << pid << " part " << partstart - buf << " " << partend - buf << endl;

  // Position the writing heads at the head of each bucket. (TODO find better
  // name than head)
  vector<char *> heads(ncpus);
  for (unsigned int i = 0; i < ncpus; i++) {
    heads[i] = bs[i].bufs[0];
  }

  // This is used for creating (name, title) tuples. (No tuple may exceed 1024
  // bytes.)
  char tmp[1024];

  // Iterate over the partitions.
  char *p = partstart, *end = partend;
  int counter = 0;
  while (p < end) {
    char *name = p;
    p = strchr(p, '\0') + 1;
    // Fill in the first part of the tuple.
    strcpy(tmp, name);
    char *subtmp = tmp + strlen(name) + 1;
    while (true) {
      char *title = p;
      p = strchr(p, '\0') + 1;

      // Fill in the second half of the tuple.
      strcpy(subtmp, title);
      size_t tmplen = subtmp + strlen(subtmp) + 2 - tmp;
      check(tmplen < 1024);
      tmp[tmplen-1] = '\0';

      // Copy the tuple into the correct local bucket.
      push_bucket(heads, bs, title, tmp, tmplen);
      counter++;

      // End of tuple?
      if (*p == '\0') {
        p++;
        break;
      }
    }
  }

  // Record the written size of each bucket.
  for (unsigned int i = 0; i < ncpus; i++) {
    bs[i].sz.back() = heads[i] - bs[i].bufs.back();
  }

  // TODO fix this log msg to sum up the sz's rather than just showing the last
  cout << "actress count " << counter << " nbytes " << bs[0].sz.back()<< endl;
}

const hmap *
movdb::build(const bucket **movbucs)
{
  vector<pthread_t> ts(ncpus);
  hmap *hs = new hmap[ncpus];
  for (unsigned int i = 0; i < ncpus; i++) {
    ts[i] = method_thread(this, &movdb::build1, i, movbucs, &hs[i]);
  }
  for (unsigned int i = 0; i < ncpus; i++) {
    void *value;
    check(pthread_join(ts[i], &value) == 0);
  }
  return hs;
}

void
movdb::build1(unsigned int pid, const bucket **movbucs, hmap *ph)
{
  hmap &h = *ph;
  h.resize(map_size);
  // Visit each bucket that's destined to us (visit each source).
  for (unsigned int i = 0; i < ncpus; i++) {
    const vector<char*>& bufs = movbucs[i][pid].bufs;
    const vector<size_t>& sz  = movbucs[i][pid].sz;
    // Iterate over the bucket.
    for (unsigned int j = 0; j < bufs.size(); j++) {
      char *p = bufs[j], *end = bufs[j] + sz[j];
      // Iterate over the chunk.
      while (p < end) {
        char *title = p;
        char *release = strchr(p, '\0') + 1;
        p = strchr(release, '\0') + 2;
        // Insert into hash map.
        h[title] = release;
      }
    }
  }
  cout << "h.size " << h.size() << endl;
}

void
actdb::probe(const hmap *hs, const bucket **actbucs, bool show_progress)
{
  vector<pthread_t> ts(ncpus);
  for (unsigned int i = 0; i < ncpus; i++) {
    ts[i] = method_thread(this, &actdb::probe1, i, &hs[i], actbucs);
  }
  for (unsigned int i = 0; i < ncpus; i++) {
    void *value;
    check(pthread_join(ts[i], &value) == 0);
  }
}

/**
 * Dummy function that is called to represent emitting a joined tuple.
 */
inline void
join(const char *movie, const char *actress)
{
  if (false) cout << "JOINED: " << movie << " WITH " << actress << endl;
}

void
actdb::probe1(unsigned int pid, const hmap *ph, const bucket **actbucs)
{
  const hmap &h = *ph;
  int hits = 0, misses = 0;
  // For each source bucket.
  for (unsigned int i = 0; i < ncpus; i++) {
    const vector<char*>& bufs = actbucs[i][pid].bufs;
    const vector<size_t>& sz  = actbucs[i][pid].sz;
    // Iterate over the bucket.
    for (unsigned int j = 0; j < bufs.size(); j++) {
      char *p = bufs[j], *end = bufs[j] + sz[j];
      // Iterate over the chunk.
      while (p < end) {
        char *name = p;
        p = strchr(p, '\0') + 1;
        while (true) {
          char *title = p;
          p = strchr(p, '\0') + 1;
          // Emit the joined tuple (if a join was possible).
          if (h.find(title) != h.end()) {
            hits++;
            join(title, name);
          } else {
            if (misses == 0) cerr << "MISS: '" << title << '\'' << endl;
            misses++;
          }
          // End of a tuple? (Don't actually need this check, since the
          // hash-partitioning "normalizes" the tuples from the actresses file.)
          if (*p == '\0') {
            p++;
            break;
          }
        }
      }
    }
  }
  cout << "cpu " << pid << " hits " << hits << " misses " << misses << endl;
}

// vim:et:sw=2:ts=2
