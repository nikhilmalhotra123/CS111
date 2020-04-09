#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>


struct termios originalconfig, newconfig;


void restoreTerminal() {
  tcsetattr(0, TCSANOW, &originalconfig);
}

void setTerminal() {
  if (tcgetattr(0, &originalconfig) != 0 || tcgetattr(0, &newconfig) != 0) {
    printf("Error getting terminal attributes");
    exit(1);
  }
  atexit(restoreTerminal);

  newconfig.c_iflag = ISTRIP;
  newconfig.c_oflag = 0;
  newconfig.c_lflag = 0;

  tcsetattr(0, TCSANOW, &newconfig);
}

int main(int argc, char **argv) {
  setTerminal();

  char buf[128];
  size_t size;
  while ((size = read(0, &buf, sizeof(char)*128)) > 0) {
    write(1, &buf, size*sizeof(char));
  }

  exit(0);
}
