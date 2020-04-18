//NAME: Nikhil Malhotra
//EMAIL: nikhilmalhotra@g.ucla.edu
//ID: 505103892

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>

struct termios originalconfig, newconfig;
char* exec_file;
int pipeChild[2];
int pipeParent[2];
int pid;
int shell_flag;

void restoreTerminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalconfig);
  if (shell_flag) {
    int status;
    if (waitpid(pid, &status, 0) != pid) {
      fprintf(stderr, "Waitpid error: %s\n", strerror(errno));
      exit(1);
    }
    if (WIFEXITED(status)) {
      fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
      exit(0);
    }
  }
}

void setTerminal() {
  if (!isatty(STDIN_FILENO)) {
    fprintf(stderr, "Not a terminal.\n");
    exit(1);
  }

  if (tcgetattr(STDIN_FILENO, &originalconfig) != 0 || tcgetattr(STDIN_FILENO, &newconfig) != 0) {
    fprintf(stderr, "Error getting terminal attributes");
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

  dup2(pipeChild[0], 0);
  dup2(pipeParent[1], 1);

  close(pipeChild[0]);
  close(pipeParent[1]);

  //Execute program
  char* args[2] = {exec_file, NULL};
  if (execvp(exec_file, args) == -1) {
    fprintf(stderr, "Failed to execute program: %s\n", strerror(errno));
    exit(1);
  }
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
      size = read(0, &buf, 256); //keyboard input which is standard input
      if (size < 0) {
        fprintf(stderr, "Failed to read from keyboard input");
        exit(1);
      }
      for(int i = 0; i < size; i++) {
        temp = buf[i];
        switch(temp) {
          case '\003':
            kill(pid, SIGINT);
            break;
          case '\004':
            close(pipeChild[1]);
            break;
          case '\012':
          case '\015':
            write(1, &crlf, 2);
            write(pipeChild[1], &lf, 1);
            break;
          default:
            write(1, &temp, 1);
            write(pipeChild[1], &buf, 1);
            break;
        }
      }
      memset(buf, 0, size);
    }

    if (fds[1].revents & POLLIN) {
      size = read(pipeParent[0], &buf, 256);
      if (size < 0) {
        fprintf(stderr, "Failed to read from shell output");
        exit(1);
      }
      for(int i = 0; i < size; i++) {
        temp = buf[i];
        switch(temp) {
          case '\012':
            write(1, &crlf, 2);
            break;
          default:
            write(1, &temp, 1);
            break;
        }
      }
      memset(buf, 0, size);
    }
    if ((POLLHUP | POLLERR) & fds[1].revents) {
      exit(0);
    }
  }
}

//Regular copy
void copy() {
  char buf[128];
  char crlf[2] = {'\015', '\012'};
  int size;
  char temp;
  while ((size = read(0, &buf, 128)) > 0) {
    for(int i = 0; i < size; i++) {
      temp = buf[i];
      switch(temp) {
        case '\004':
          exit(0);
          break;
        case '\012':
        case '\015':
          write(1, &crlf, 2);
          break;
        default:
          write(1, &temp, 1);
          break;
      }
    }
    memset(buf, 0, size);
  }
}

int main(int argc, char **argv) {
  //Process args
  int c;
  shell_flag = 0;

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
        exit(1);
    }
  }

  //Set terminal attributes
  setTerminal();

  if (shell_flag) {
    //Create pipes and fork
    if (pipe(pipeChild) != 0) {
      fprintf(stderr, "Error creating child pipe: %s\n", strerror(errno));
      exit(1);
    }
    if (pipe(pipeParent) != 0) {
      fprintf(stderr, "Error creating parent pipe: %s\n", strerror(errno));
      exit(1);
    }
    pid = fork();
    if (pid < -1) {
      fprintf(stderr, "Error while forking: %s\n", strerror(errno));
      exit(1);
    }
    else if (pid == 0) {
      runChild();
    }
    else {
      runParent();
    }
  }

  copy();
  exit(0);
}
