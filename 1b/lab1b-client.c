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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

struct termios originalconfig, newconfig;
int port;
int sockfd;

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

void connectToServer() {
  struct sockaddr_in serv_addr;
  struct hostent *server;
  char buffer[256];

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Error opening socket: %s\n", strerror(errno));
    exit(1);
  }

  server = gethostbyname("localhost");

  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
  serv_addr.sin_port = htons(port);

  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error connecting to socket: %s\n", strerror(errno));
    exit(1);
  }
  //Succesfully connected to server
}

int main(int argc, char **argv) {
  //Process args
  int c;

  while(1) {
    static struct option long_options[] = {
        {"port",  required_argument,  0,  'p'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "p:l:", long_options, &option_index);

    if (c == -1)
      break;

    switch(c) {
      case 'p':
        port = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "--port=port#  Port Number\tRequired\n");
        fprintf(stderr, "--log=logfile Log File   \tOptional\n");
        exit(1);
    }
  }

  connectToServer();

  printf("Please enter the message: ");
  bzero(buffer,256);
  read(0, buffer, 10);
  int n = write(sockfd,buffer,strlen(buffer));
  if (n < 0)
       printf("ERROR writing to socket");
  bzero(buffer,256);
  n = read(sockfd,buffer,255);
  if (n < 0)
       printf("ERROR reading from socket");
  printf("%s\n",buffer);
}
