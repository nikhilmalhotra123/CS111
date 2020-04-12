#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>


struct termios originalconfig, newconfig;
char* exec_file;
int pipeChild[2];
int pipeParent[2];

void restoreTerminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalconfig);
}

void setTerminal() {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Not a terminal.\n");
    exit(1);
  }

  if (tcgetattr(STDIN_FILENO, &originalconfig) != 0 || tcgetattr(STDIN_FILENO, &newconfig) != 0) {
    printf("Error getting terminal attributes");
    exit(1);
  }
  atexit(restoreTerminal);

  newconfig.c_iflag &= ISTRIP;
  newconfig.c_oflag = 0;
  newconfig.c_lflag = 0;

  tcsetattr(STDIN_FILENO, TCSANOW, &newconfig);
}

void runChild() {
  close(pipeChild[1]); //close child output
  close(pipeParent[0]); //close input from parent

  execvp(exec_file, NULL);
}

void runParent() {
  close(pipeChild[0]); //close input from child
  close(pipeParent[1]); //close parent output

}

void copy() {
  // Copy
  char buf[256];
  char crlf[2] = {'\015', '\012'};
  int size;
  while ((size = read(STDIN_FILENO, &buf, 256)) > 0) {
    for(int i = 0; i < size; i++) {
      switch(buf[i]) {
        case '\004':
          exit(0);
          break;
        case '\012':
        case '\015':
          write(1, &crlf, 2);
          break;
        default:
          write(1, &buf, size);
          break;
      }
    }
  }
}

int main(int argc, char **argv) {
  //Process args
  int c;
  int shell_flag = 0;

  while(1) {
    static struct option long_options[] = {
        {"shell",  optional_argument,  0,  's'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "s:", long_options, &option_index);
    if (c == -1)
        break;

    switch(c) {
      case 's':
        shell_flag = 1;
        exec_file = optarg;
        fprintf(stderr, "shell\n");
        fprintf(stderr, "%s\n", exec_file);
        break;
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--shell=<program> Shell program\n");
    }
  }

  setTerminal();

  if (shell_flag) {
    if (pipe(pipeChild) != 0) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
    if (pipe(pipeParent) != 0) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
    int output = fork();
    if (output < -1) {
      fprintf(stderr, "%s\n", strerror(errno));
      exit(1);
    }
    else if (output == 0) {
      runChild();
    }
    else {
      runParent();
    }
  }

  copy();
  exit(0);
}
