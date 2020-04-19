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

int port;
int shell_flag;
char* exec_file;
int newsockfd;

void openSocket() {
  struct sockaddr_in serv_addr, cli_addr;

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) {
    fprintf(stderr, "Error opening socket: %s\n", strerror(errno));
    exit(1);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
    exit(1);
  }

  listen(sockfd, 5);

  socklen_t cli_len = sizeof(cli_addr);
  newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
  if (newsockfd < 0) {
    fprintf(stderr, "Error opening socket: %s\n", strerror(errno));
    exit(1);
  }
  //Succesfully connected to client
}

int main(int argc, char **argv) {
  //Process args
  int c;
  shell_flag = 0;

  while(1) {
    static struct option long_options[] = {
        {"port",  required_argument,  0,  'p'},
        {"shell",  optional_argument,  0,  's'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "s:", long_options, &option_index);

    if (c == -1)
      break;

    switch(c) {
      case 'p':
        port = atoi(optarg);
        break;
      case 's':
        shell_flag = 1;
        exec_file = optarg;
        break;
      default:
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "--port=port#      Port Number  \tRequired\n");
        fprintf(stderr, "--shell=<program> Shell program\tOptional\n");
        exit(1);
    }
  }

  openSocket();

  char buffer[256];
  int size;
  bzero(buffer, 256);

  while ((size = read(newsockfd, &buffer, 255)) > 0) {
    printf("got message: %s\n", buffer);
    write(newsockfd,"I got your message",18);
  }
}
