//NAME: Nikhil Malhotra
//EMAIL: nikhilmalhotra@g.ucla.edu
//ID: 505103892

#include <stdio.h>
#include <stdlib.h>
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
#include <fcntl.h>
#include <assert.h>
#include "zlib.h"

int port;
int shell_flag;
int compress_flag;
char* exec_file;
int pipeChild[2];
int pipeParent[2];
int pid;
int newsockfd;
z_stream from_shell;
z_stream from_client;

void signal_handler(int sigNum){
    if(sigNum == SIGPIPE){
        exit(0);
    }
    if(sigNum == SIGINT){
        kill(pid, SIGINT);
    }
}

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

void shutdownServer() {
  int status;
  if (waitpid(pid, &status, 0) != pid) {
    fprintf(stderr, "Waitpid error: %s\n", strerror(errno));
    exit(1);
  }
  if (WIFEXITED(status)) {
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(status), WEXITSTATUS(status));
    exit(0);
  }
  exit(0);
}

void setupCompression() {
  from_shell.zalloc = Z_NULL;
  from_shell.zfree = Z_NULL;
  from_shell.opaque = Z_NULL;
  int ret = deflateInit(&from_shell, Z_DEFAULT_COMPRESSION);
  if (ret < 0) {
    fprintf(stderr, "Failed compression init\n");
    exit(1);
  }

  from_client.zalloc = Z_NULL;
  from_client.zfree = Z_NULL;
  from_client.opaque = Z_NULL;
  ret = inflateInit(&from_client);
  if (ret < 0) {
    fprintf(stderr, "Failed decompression init\n");
    exit(1);
  }
}

void cleanupCompression() {
  deflateEnd(&from_shell);
  inflateEnd(&from_client);
}

int compression(int bytes_to_compress, char* buffer, char* compressedBuffer, int compressedBufferSize) {
  from_shell.avail_in = bytes_to_compress;
  from_shell.next_in = (Bytef *) buffer;
  from_shell.avail_out = compressedBufferSize;
  from_shell.next_out = (Bytef *) compressedBuffer;

  do {
    int ret = deflate(&from_shell, Z_SYNC_FLUSH);
    if (ret == Z_STREAM_ERROR) {
      fprintf(stderr, "Compressing stream state inconsistent: %s\n", from_shell.msg);
      exit(1);
    }
  } while (from_shell.avail_in > 0);

  return compressedBufferSize - from_shell.avail_out;
}

int decompression(int bytes_to_decompress, char* buffer, char* decompressedBuffer, int decompressedBufferSize) {
  from_client.avail_in = bytes_to_decompress;
  from_client.next_in = (Bytef *) buffer;
  from_client.avail_out = decompressedBufferSize;
  from_client.next_out = (Bytef *) decompressedBuffer;

  do {
    int ret = inflate(&from_client, Z_SYNC_FLUSH);
    if (ret == Z_STREAM_ERROR) {
      fprintf(stderr, "Decompressing stream state inconsistent: %s\n", from_client.msg);
      exit(1);
    }
  } while (from_client.avail_in > 0);

  return decompressedBufferSize - from_client.avail_out;
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

  fds[0].fd = newsockfd; //client input
  fds[0].events = POLLIN | POLLHUP | POLLERR;

  fds[1].fd = pipeParent[0]; //shell output
  fds[1].events = POLLIN | POLLHUP | POLLERR;

  int size;
  char buf[256];
  char compressbuf[256];
  char crlf[2] = {'\015', '\012'};
  char lf[1] = {'\012'};
  char temp;

  while (1) {
    ret = poll(fds, 2, 0);
    if (ret == -1) {
      fprintf(stderr, "Error while polling: %s\n", strerror(errno));
    }

    if (fds[0].revents & POLLIN) {
      size = read(fds[0].fd, &buf, 256); //client input
      if (size < 0) {
        fprintf(stderr, "Failed to read from client");
        exit(1);
      }
      if (compress_flag) {
        char tempbuf[256];
        size = decompression(size, buf, tempbuf, 256);
        memcpy(buf, tempbuf, size);
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
            write(pipeChild[1], &lf, 1);
            break;
          default:
            write(pipeChild[1], &buf, 1);
            break;
        }
      }
      memset(buf, 0, size);
    }

    if (fds[1].revents & POLLIN) { //shell output
      size = read(pipeParent[0], &buf, 256);
      if (size < 0) {
        fprintf(stderr, "Failed to read from shell output");
        exit(1);
      }
      int j = 0;
      for(int i = 0; i < size; i++) {
        temp = buf[i];
        switch(temp) {
          case '\012':
            if (!compress_flag) {
              write(newsockfd, &crlf, 2);
            }
            else {
              compressbuf[j] = crlf[0];
              j += 1;
              compressbuf[j] = crlf[1];
            }
            break;
          default:
            if (!compress_flag) {
              write(newsockfd, &temp, 1);
            }
            else
              compressbuf[j] = temp;
            break;
        }
        j++;
      }
      if (compress_flag) {
        char tempbuf[256];
        size = compression(j, compressbuf, tempbuf, 256);
        write(newsockfd, &tempbuf, size);
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
  char buf[256];
  char compressbuf[256];
  char crlf[2] = {'\015', '\012'};
  int size;
  char temp;
  while ((size = read(0, &buf, 256)) > 0) {
    int j = 0;
    for(int i = 0; i < size; i++) {
      temp = buf[i];
      switch(temp) {
        case '\004':
          exit(0);
        case '\012':
          if (!compress_flag) {
            write(newsockfd, &crlf, 2);
          }
          else {
            compressbuf[j] = crlf[0];
            j += 1;
            compressbuf[j] = crlf[1];
          }
          break;
        default:
          if (!compress_flag) {
            write(newsockfd, &temp, 1);
          }
          else
            compressbuf[j] = temp;
          break;
      }
      j++;
    }
    if (compress_flag) {
      char tempbuf[256];
      size = compression(j, compressbuf, tempbuf, 256);
      write(newsockfd, &tempbuf, size);
    }
    memset(buf, 0, size);
  }
}

int main(int argc, char **argv) {
  signal(SIGPIPE, signal_handler);
  signal(SIGINT, signal_handler);
  atexit(shutdownServer);

  //Process args
  int c;
  int port_flag = 0;
  compress_flag = 0;
  shell_flag = 0;

  while(1) {
    static struct option long_options[] = {
        {"port",  required_argument,  0,  'p'},
        {"shell",  required_argument,  0,  's'},
        {"compress",    no_argument,   0,  'c'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "p:s:", long_options, &option_index);

    if (c == -1)
      break;

    switch(c) {
      case 'p':
        port = atoi(optarg);
        port_flag = 1;
        break;
      case 's':
        shell_flag = 1;
        exec_file = optarg;
        break;
      case 'c':
        compress_flag = 1;
        break;
      default:
        fprintf(stderr, "Invalid arguments2\n");
        fprintf(stderr, "--port=port#      Port Number     Required\n");
        fprintf(stderr, "--shell=<program> Shell program   Optional\n");
        fprintf(stderr, "--compress    Enable Compression  Optional");
        exit(1);
    }
  }
  if (!port_flag) {
    fprintf(stderr, "Invalid arguments1\n");
    fprintf(stderr, "--port=port#      Port Number     Required\n");
    fprintf(stderr, "--shell=<program> Shell program   Optional\n");
    fprintf(stderr, "--compress    Enable Compression  Optional");
    exit(1);
  }

  openSocket();

  if (compress_flag) {
    setupCompression();
    atexit(cleanupCompression);
  }

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
