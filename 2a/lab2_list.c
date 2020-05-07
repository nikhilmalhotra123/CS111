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

char* protected;
char* syncType;
pthread_t *threads;

SortedList_t* head;
SortedListElement_t* list;

struct timespec start, stop;
long runtime;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int spinLock = 0;

void signal_handler(int sigNum) {
    if (sigNum == SIGSEGV) {
      fprintf(stderr, "Caught segmentation fault");
    }
    exit(1);
}

char* genKey() {
  char* key = (char*) malloc(2*sizeof(char));
  key[0] = (char) (rand() % 26) + 'a';
  key[1] = '\0';
  return key;
}

void* modifyList(void* arg) {
  int num = *((int *) arg);

  for (int i = num; i < numElements; i+=numOfThreads) {
    switch (*syncType) {
      case 'm':
        pthread_mutex_lock(&mutex);
        SortedList_insert(head, &list[i]);
        pthread_mutex_unlock(&mutex);
        break;
      case 's':
        while(__sync_lock_test_and_set(&spinLock, 1));
        SortedList_insert(head, &list[i]);
        __sync_lock_release(&spinLock);
        break;
      default:
        SortedList_insert(head, &list[i]);
        break;
    }
  }

  int len;
  switch (*syncType) {
    case 'm':
      pthread_mutex_lock(&mutex);
      len = SortedList_length(head);
      if (len == -1) {
        fprintf(stderr, "Error on getting length");
        exit(2);
      }
      pthread_mutex_unlock(&mutex);
      break;
    case 's':
      while(__sync_lock_test_and_set(&spinLock, 1));
      len = SortedList_length(head);
      if (len == -1) {
        fprintf(stderr, "Error on getting length");
        exit(2);
      }
      __sync_lock_release(&spinLock);
      break;
    default:
      len = SortedList_length(head);
      if (len == -1) {
        fprintf(stderr, "Error on getting length");
        exit(2);
      }
      break;
  }


  SortedListElement_t* search;
  for (int i = num; i < numElements; i+=numOfThreads) {
    switch (*syncType) {
      case 'm':
        pthread_mutex_lock(&mutex);
        search = SortedList_lookup(head, list[i].key);
        if (search == NULL) {
          fprintf(stderr, "Error on lookup");
          exit(2);
        }
        if (SortedList_delete(search)) {
          fprintf(stderr, "Error deleting element");
          exit(2);
        }
        pthread_mutex_unlock(&mutex);
        break;
      case 's':
        while(__sync_lock_test_and_set(&spinLock, 1));
        search = SortedList_lookup(head, list[i].key);
        if (search == NULL) {
          fprintf(stderr, "Error on lookup");
          exit(2);
        }
        if (SortedList_delete(search)) {
          fprintf(stderr, "Error deleting element");
          exit(2);
        }
        __sync_lock_release(&spinLock);
        break;
      default:
        search = SortedList_lookup(head, list[i].key);
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

  printf("%s,%d,%d,%d,%d,%ld,%ld\n", tag, numOfThreads, iterations, 1, operations, runtime, runtime/operations);
}

int main(int argc, char **argv) {
  //Process args
  int c;
  int i;
  numOfThreads = 1;
  iterations = 1;
  opt_yield = 0;
  syncType = "";

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
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--threads=#, default 1\n");
        fprintf(stderr, "--iterations=#, default 1\n");
        fprintf(stderr, "--yield=[idl], default no yield\n");
        exit(1);
    }
  }

  signal(SIGSEGV, signal_handler);

  numElements = numOfThreads * iterations;
  head = malloc(sizeof(SortedList_t));
  head->next = head;
  head->prev = head;
  head->key = NULL;

  list = malloc(numElements * sizeof(SortedListElement_t));
  for (int i = 0; i < numElements; i++) {
    list[i].key = genKey();
  }

  runThreads();

  printCustomOutput();

  exit(0);
}
