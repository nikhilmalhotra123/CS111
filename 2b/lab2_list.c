//NAME: Nikhil Malhotra
//EMAIL: nikhilmalhotra@g.ucla.edu
//ID: 505103892

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include "SortedList.h"
#include <signal.h>

int numOfThreads;
int iterations;
int opt_yield;
int numElements;
int numOfLists;

char* protected;
char* syncType;
pthread_t *threads;

struct timespec start, stop;
long runtime;
long locktime;

SortedList_t* heads;
SortedListElement_t* elements;
pthread_mutex_t* mutexes;
int* spinLocks;

void signal_handler(int sigNum) {
    if (sigNum == SIGSEGV) {
      fprintf(stderr, "Caught segmentation fault");
    }
    exit(1);
}

int hash(const char* key) {
	return key[0] % numOfLists;
}

char* genKey() {
  char* key = (char*) malloc(2*sizeof(char));
  key[0] = (char) (rand() % 26) + 'a';
  key[1] = '\0';
  return key;
}

void* modifyList(void* arg) {
  int num = *((int *) arg);
  struct timespec lockstart, lockstop;

  for (int i = num; i < numElements; i+=numOfThreads) {
    switch (*syncType) {
      case 'm':
        if(clock_gettime(CLOCK_REALTIME, &lockstart) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        pthread_mutex_lock(&mutexes[hash(elements[i].key)]);
        if(clock_gettime(CLOCK_REALTIME, &lockstop) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        locktime += ((lockstop.tv_sec - lockstart.tv_sec)*1000000000) + (lockstop.tv_nsec - lockstart.tv_nsec);

        SortedList_insert(&heads[hash(elements[i].key)], &elements[i]);
        pthread_mutex_unlock(&mutexes[hash(elements[i].key)]);
        break;
      case 's':
        if(clock_gettime(CLOCK_REALTIME, &lockstart) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        while(__sync_lock_test_and_set(&spinLocks[hash(elements[i].key)], 1));
        if(clock_gettime(CLOCK_REALTIME, &lockstop) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        locktime += ((lockstop.tv_sec - lockstart.tv_sec)*1000000000) + (lockstop.tv_nsec - lockstart.tv_nsec);

        SortedList_insert(&heads[hash(elements[i].key)], &elements[i]);
        __sync_lock_release(&spinLocks[hash(elements[i].key)]);
        break;
      default:
        SortedList_insert(&heads[hash(elements[i].key)], &elements[i]);
        break;
    }
  }

  int len = 0;
  switch (*syncType) {
    case 'm':
      for (int i = 0; i < numOfLists; i++) {
        if(clock_gettime(CLOCK_REALTIME, &lockstart) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        pthread_mutex_lock(&mutexes[i]);
        if(clock_gettime(CLOCK_REALTIME, &lockstop) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        locktime += ((lockstop.tv_sec - lockstart.tv_sec)*1000000000) + (lockstop.tv_nsec - lockstart.tv_nsec);

        int temp = SortedList_length(&heads[i]);
        if (temp == -1) {
          fprintf(stderr, "Error on getting length");
          exit(2);
        }
        pthread_mutex_unlock(&mutexes[i]);
        len += temp;
      }
      break;
    case 's':
      for (int i = 0; i < numOfLists; i++) {
        if(clock_gettime(CLOCK_REALTIME, &lockstart) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        while(__sync_lock_test_and_set(&spinLocks[i], 1));
        if(clock_gettime(CLOCK_REALTIME, &lockstop) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        locktime += ((lockstop.tv_sec - lockstart.tv_sec)*1000000000) + (lockstop.tv_nsec - lockstart.tv_nsec);

        int temp = SortedList_length(&heads[i]);
        if (temp == -1) {
          fprintf(stderr, "Error on getting length");
          exit(2);
        }
        __sync_lock_release(&spinLocks[i]);
        len += temp;
      }
      break;
    default:
      for (int i = 0; i < numOfLists; i++) {
        int temp = SortedList_length(&heads[i]);
        if (temp == -1) {
          fprintf(stderr, "Error on getting length");
          exit(2);
        }
        len += temp;
      }
      break;
  }


  SortedListElement_t* search;
  for (int i = num; i < numElements; i+=numOfThreads) {
    switch (*syncType) {
      case 'm':
        if(clock_gettime(CLOCK_REALTIME, &lockstart) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        pthread_mutex_lock(&mutexes[hash(elements[i].key)]);
        if(clock_gettime(CLOCK_REALTIME, &lockstop) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        locktime += ((lockstop.tv_sec - lockstart.tv_sec)*1000000000) + (lockstop.tv_nsec - lockstart.tv_nsec);

        search = SortedList_lookup(&heads[hash(elements[i].key)], elements[i].key);
        if (search == NULL) {
          fprintf(stderr, "Error on lookup");
          exit(2);
        }
        if (SortedList_delete(search)) {
          fprintf(stderr, "Error deleting element");
          exit(2);
        }
        pthread_mutex_unlock(&mutexes[hash(elements[i].key)]);
        break;
      case 's':
        if(clock_gettime(CLOCK_REALTIME, &lockstart) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        while(__sync_lock_test_and_set(&spinLocks[hash(elements[i].key)], 1));
        if(clock_gettime(CLOCK_REALTIME, &lockstop) == -1) {
            fprintf(stderr, "Failed to get time");
            exit(1);
        }
        locktime += ((lockstop.tv_sec - lockstart.tv_sec)*1000000000) + (lockstop.tv_nsec - lockstart.tv_nsec);

        search = SortedList_lookup(&heads[hash(elements[i].key)], elements[i].key);
        if (search == NULL) {
          fprintf(stderr, "Error on lookup");
          exit(2);
        }
        if (SortedList_delete(search)) {
          fprintf(stderr, "Error deleting element");
          exit(2);
        }
        __sync_lock_release(&spinLocks[hash(elements[i].key)]);
        break;
      default:
        search = SortedList_lookup(&heads[hash(elements[i].key)], elements[i].key);
        if (search == NULL) {
          fprintf(stderr, "Error on lookup");
          exit(2);
        }
        if (SortedList_delete(search)) {
          fprintf(stderr, "Error deleting element");
          exit(2);
        }
        break;
    }
    //free(list[i].key);
  }
  return NULL;
}

void runThreads() {

  threads = malloc(sizeof(pthread_t)*numOfThreads);
  if (threads == NULL) {
    fprintf(stderr, "Malloc failed.");
    exit(1);
  }
  int* borders = (int*) malloc(numOfThreads * sizeof(int));

  if(clock_gettime(CLOCK_REALTIME, &start) == -1) {
      fprintf(stderr, "Failed to get time");
      exit(1);
  }
  
  for (int i = 0; i < numOfThreads; i++) {
    borders[i] = i;
    if(pthread_create(&threads[i], NULL, (void *) modifyList, &borders[i]) != 0) {
      fprintf(stderr, "Failed to create thread");
      exit(1);
    }
  }
  for (int i = 0; i < numOfThreads; i++) {
    pthread_join(threads[i], NULL);
  }
  //free(threads);

  if(clock_gettime(CLOCK_REALTIME, &stop) == -1) {
      fprintf(stderr, "Failed to get time");
      exit(1);
  }
  runtime = ((stop.tv_sec - start.tv_sec)*1000000000) + (stop.tv_nsec - start.tv_nsec);
}

void printCustomOutput() {
  int operations = numOfThreads*iterations*3;

  char tag[2048] = "";
  char yield[7] = "-";
  char* syncString = "-none";

  if (opt_yield)
    strcat(yield, protected);
  else
    strcat(yield, "none");
  if (strncmp(syncType, "m", 1) == 0)
    syncString = "-m";
  else if (strncmp(syncType, "s", 1) == 0)
    syncString = "-s";

  strcat(tag, "list");
  strcat(tag, yield);
  strcat(tag, syncString);

  printf("%s,%d,%d,%d,%d,%ld,%ld,%ld\n", tag, numOfThreads, iterations, numOfLists, operations, runtime, runtime/operations, locktime/operations);
}

int main(int argc, char **argv) {
  //Process args
  int c;
  int i;
  numOfThreads = 1;
  numOfLists = 1;
  iterations = 1;
  opt_yield = 0;
  syncType = "";
  locktime = 0;

  while(1) {
    static struct option long_options[] = {
        {"threads",  optional_argument,  0,  't'},
        {"iterations",  optional_argument,  0,  'i'},
        {"yield",  optional_argument,  0,  'y'},
        {"sync",  optional_argument,  0,  's'},
        {"lists", optional_argument, 0, 'l'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "t:i:ys:l:", long_options, &option_index);
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
        protected = optarg;
        if ((int) strlen(protected) > 3) {
          fprintf(stderr, "Too many yield arguments. Max 3.");
          exit(1);
        }
        for (i = 0; i < (int) strlen(protected); i++) {
          switch (protected[i]) {
            case 'i':
              opt_yield |= INSERT_YIELD;
              break;
            case 'd':
              opt_yield |= DELETE_YIELD;
              break;
            case 'l':
              opt_yield |= LOOKUP_YIELD;
              break;
            default:
              fprintf(stderr, "Invalid yield argument");
              exit(1);
          }
        }
        break;
      case 's':
        syncType = optarg;
        if (strncmp(syncType, "m", 1) != 0 && strncmp(syncType, "s", 1) != 0) {
          fprintf(stderr, "Invalid synchronization. Must be m or s.");
        }
        break;
      case 'l':
        numOfLists = atoi(optarg);
        if (numOfLists <= 0) {
          fprintf(stderr, "Invalid number of lists. Must be at least 1");
          exit(1);
        }
        break;
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--threads=#, default 1\n");
        fprintf(stderr, "--iterations=#, default 1\n");
        fprintf(stderr, "--yield=[idl], default no yield\n");
        fprintf(stderr, "--lists=#, default 1\n");
        exit(1);
    }
  }
  signal(SIGSEGV, signal_handler);

  numElements = numOfThreads * iterations;
  heads = malloc(numOfLists * sizeof(SortedList_t));
  for (int i = 0; i < numOfLists; i++) {
    heads[i].next = &heads[i];
    heads[i].prev = &heads[i];
    heads[i].key = NULL;
  }

  elements = malloc(numElements * sizeof(SortedListElement_t));
  for (int i = 0; i < numElements; i++) {
	elements[i].key = genKey();
  }

  switch (*syncType) {
    case 'm':
      mutexes = malloc(numOfLists * sizeof(pthread_mutex_t));
      for (int i = 0; i < numOfLists; i++) {
        pthread_mutex_init(&mutexes[i], NULL);
      }
      break;
    case 's':
      spinLocks = malloc(numOfLists * sizeof(int));
      for (int i = 0; i < numOfLists; i++) {
        spinLocks[i] = 0;
      }
      break;
  }

  runThreads();

  printCustomOutput();

  exit(0);
}
