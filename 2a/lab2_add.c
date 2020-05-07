#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>

int numOfThreads;
int iterations;
int opt_yield;
char* protected;
pthread_t *threads;
long long counter;
long runtime;

struct timespec start, stop;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int spinLock = 0;

void add(long long *pointer, long long value) {
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
}

void threadAdd() {
  for (int i = 0; i < iterations; i++) {
    add(&counter, 1);
  }
  for (int i = 0; i < iterations; i++) {
    add(&counter, -1);
  }
}

void addMutex(long long *pointer, long long value) {
  pthread_mutex_lock(&mutex);
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  pthread_mutex_unlock(&mutex);
}

void threadAddMutex() {
  for (int i = 0; i < iterations; i++) {
    addMutex(&counter, 1);
  }
  for (int i = 0; i < iterations; i++) {
    addMutex(&counter, -1);
  }
}

void addSpin(long long *pointer, long long value) {
  while(__sync_lock_test_and_set(&spinLock, 1));
  long long sum = *pointer + value;
  if (opt_yield)
    sched_yield();
  *pointer = sum;
  __sync_lock_release(&spinLock);
}

void threadAddSpin() {
  for (int i = 0; i < iterations; i++) {
    addSpin(&counter, 1);
  }
  for (int i = 0; i < iterations; i++) {
    addSpin(&counter, -1);
  }
}

void addAtomic(long long *pointer, long long value) {
  long long old, new;
  do {
    old = counter;
    new = old + value;
    if (opt_yield)
      sched_yield();
  } while (__sync_val_compare_and_swap(pointer, old, new) != old);
}

void threadAddAtomic() {
  for (int i = 0; i < iterations; i++) {
    addAtomic(&counter, 1);
  }
  for (int i = 0; i < iterations; i++) {
    addAtomic(&counter, -1);
  }
}

void runThreads() {
  void (*addfunc) = &threadAdd;
  if (strncmp(protected, "m", 1) == 0)
    addfunc = &threadAddMutex;
  else if (strncmp(protected, "s", 1) == 0)
    addfunc = &threadAddSpin;
  else if (strncmp(protected, "c", 1) == 0)
    addfunc = &threadAddAtomic;

  threads = malloc(sizeof(pthread_t)*numOfThreads);
  if (threads == NULL) {
    fprintf(stderr, "Malloc failed.");
    exit(1);
  }

  if(clock_gettime(CLOCK_REALTIME, &start) == -1) {
      fprintf(stderr, "Failed to get time");
      exit(1);
  }

  for (int i = 0; i < numOfThreads; i++) {
    if(pthread_create(&threads[i], NULL, (void *) addfunc, NULL) != 0) {
      fprintf(stderr, "Failed to create thread");
      exit(1);
    }
  }
  for (int i = 0; i < numOfThreads; i++) {
    pthread_join(threads[i], NULL);
  }

  if(clock_gettime(CLOCK_REALTIME, &stop) == -1) {
      fprintf(stderr, "Failed to get time");
      exit(1);
  }
  runtime = ((stop.tv_sec - start.tv_sec)*1000000000) + (stop.tv_nsec - start.tv_nsec);
}

void printCustomOutput() {
  int operations = numOfThreads*iterations*2;

  char tag[2048] = "";
  char* yield = "";
  char* sync = "-none";

  if (opt_yield)
    yield = "-yield";

  if (strncmp(protected, "m", 1) == 0)
    sync = "-m";
  else if (strncmp(protected, "s", 1) == 0)
    sync = "-s";
  else if (strncmp(protected, "c", 1) == 0)
    sync = "-c";

  strcat(tag, "add");
  strcat(tag, yield);
  strcat(tag, sync);

  printf("%s,%d,%d,%d,%ld,%ld,%lld\n", tag, numOfThreads, iterations, operations, runtime, runtime/operations, counter);
}

int main(int argc, char **argv) {
  //Process args
  int c;
  numOfThreads = 1;
  iterations = 1;
  opt_yield = 0;
  protected = "a";

  while(1) {
    static struct option long_options[] = {
        {"threads",  optional_argument,  0,  't'},
        {"iterations",  optional_argument,  0,  'i'},
        {"yield",  optional_argument,  0,  'y'},
        {"sync",  optional_argument,  0,  's'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "t:i:ys:", long_options, &option_index);
    if (c == -1)
        break;

    switch(c) {
      case 't':
        numOfThreads = atoi(optarg);
        if (numOfThreads <= 0) {
          fprintf(stderr, "Invalid number of threads. Must be at least 1");
          exit(1);
        }
        break;
      case 'i':
        iterations = atoi(optarg);
        if (iterations < 0) {
          fprintf(stderr, "Invalid number of iterations. Must be at least 1");
          exit(1);
        }
        break;
      case 'y':
        opt_yield = 1;
        break;
      case 's':
        protected = optarg;
        if (strncmp(protected, "m", 1) != 0 && strncmp(protected, "s", 1) != 0 && strncmp(protected, "c", 1) != 0) {
          fprintf(stderr, "Invalid synchronization. Must be m, s, or c.");
        }
        break;
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--threads=#, default 1\n");
        fprintf(stderr, "--iterations=#, default 1\n");
        fprintf(stderr, "--yield, default no yield\n");
        fprintf(stderr, "--sync, default none");
        exit(1);
    }
  }
  runThreads();

  printCustomOutput();

  exit(0);
}
