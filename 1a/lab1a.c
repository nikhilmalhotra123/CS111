#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <poll.h>


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

  dup2(pipeChild[1], 1);
  dup2(pipeParent[0], 0);

  close(pipeChild[1]);
  close(pipeParent[0]);

  char* args[2] = {exec_file, NULL};
  execvp(exec_file, args);
}

void runParent() {
  close(pipeChild[0]); //close input from child
  close(pipeParent[1]); //close parent output

  struct pollfd fds[2];
  int ret;

  fds[0].fd = 0; //standard input
  fds[0].events = POLLIN | POLLHUP | POLLERR;

  fds[1].fd = pipeParent[0]; //shell output
  fds[1].events = POLLIN | POLLHUP | POLLERR;

  int size;
  char buf[256];
  char crlf[2] = {'\015', '\012'};
  char lf[1] = {'\012'};
  char temp;
  while (1) {
    ret = poll(fds, 2, 0);
    if (ret == -1) {
      fprintf(stderr, "Error while polling: %s\n", strerror(errno));
    }

    if (fds[0].revents & POLLIN) {
      fprintf(stderr, "here");
      size = read(0, &buf, 256); //keyboard input which is standard input
      if (size < 0) {
        fprintf(stderr, "Failed to read from keyboard input");
        exit(1);
      }
      for(int i = 0; i < size; i++) {
        switch(buf[i]) {
          case '\003':
            //kill(); TODO: Fix this
            break;
          case '\004':
            exit(0);
            break;
          case '\012':
          case '\015':
            write(1, &crlf, 2);
            write(pipeChild[1], &lf, 1);
            break;
          default:
            temp = buf[i];
            write(1, &temp, size);
            write(pipeChild[1], &buf, size);
            break;
        }
      }
    }

    if (fds[1].revents & POLLIN) {
      fprintf(stderr, "here2");
      size = read(0, &buf, 256);
      if (size < 0) {
        fprintf(stderr, "Failed to read from keyboard input");
        exit(1);
      }
      for(int i = 0; i < size; i++) {
        switch(buf[i]) {
          case '\012':
            write(1, &crlf, 2);
            break;
          default:
            write(1, &buf, size);
            break;
        }
      }
    }

    if ((POLLHUP | POLLERR) & fds[1].revents) {
      fprintf(stderr, "here3");
      exit(0);
    }
  }
}

void copy() {
  // Copy
  char buf[128];
  char crlf[2] = {'\015', '\012'};
  int size;
  while ((size = read(STDIN_FILENO, &buf, 128)) > 0) {
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
        break;
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--shell=<program> Shell program\n");
    }
  }

  setTerminal();

  if (shell_flag) {
    if (pipe(pipeChild) != 0) {
      fprintf(stderr, "Error creating child pipe: %s\n", strerror(errno));
      exit(1);
    }
    if (pipe(pipeParent) != 0) {
      fprintf(stderr, "Error creating parent pipe: %s\n", strerror(errno));
      exit(1);
    }
    int output = fork();
    if (output < -1) {
      fprintf(stderr, "Error while forking: %s\n", strerror(errno));
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
