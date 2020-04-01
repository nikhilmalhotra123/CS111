#include <stdio.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

void signal_handler() {
    signal(SIGSEGV, SIG_DFL);
    fprintf(stderr, "Segmentation fault due to segfault flag: %s\n", strerror(errno));
    exit(4);
}

void cause_seg() {
	char* thing = NULL;
	char b = *thing;
}

int main(int argc, char **argv) {
  int c;
  static int seg_flag = 0;
  static int catch_flag = 0;
  while (1) {
    static struct option long_options[] = {
        {"input_file",  required_argument,  0,  'i'},
        {"output_file", required_argument,  0,  'o'},
        {"segfault",    no_argument,        &seg_flag,  1},
        {"catch",       no_argument,        &catch_flag,  1},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "i:o:", long_options, &option_index);
    if (c == -1)
        break;

    int input;
    int output;
    switch (c) {
      case 0:
        break;
      case 'i':
        input = open(optarg, O_RDONLY);
        if (input >= 0) {
          close(0);
          dup(input);
          close(input);
        }
        else {
          fprintf(stderr, "%s\n", strerror(errno));
          exit(2);
        }
        break;
      case 'o':
        output = open(optarg, O_WRONLY | O_CREAT, 0777);
        if (output >= 0) {
          close(1);
          dup(output);
          close(output);
        }
        else {
          fprintf(stderr, "%s\n", strerror(errno));
          exit(3);
        }
        break;
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--input_file=<filename> File to replace standard input\n");
        fprintf(stderr, "--output_file=<filename> File to replace standard output\n");
        fprintf(stderr, "--segfault Force a segmentation fault\n");
        fprintf(stderr, "--catch Catch a forced segmentation fault\n");
        exit(1);
      }
  }
  if (seg_flag) {
    if (catch_flag) {
      signal(SIGSEGV, signal_handler);
    }
    cause_seg();
    //char* thing = NULL;
    //printf("%c", *thing);
  }
  char buf;
  size_t size;
  while ((size = read(0, &buf, 1)) > 0) {
    write(1, &buf, size);
  }
}
