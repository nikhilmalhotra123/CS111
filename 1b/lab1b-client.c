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
#include <fcntl.h>
#include <assert.h>
#include "zlib.h"

struct termios originalconfig, newconfig;
int port;
int sockfd;
int logfile;
int log_flag, compress_flag;
z_stream from_input;
z_stream from_server;

void restoreTerminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &originalconfig);
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

void setupCompression() {
  from_input.zalloc = Z_NULL;
  from_input.zfree = Z_NULL;
  from_input.opaque = Z_NULL;
  int ret = deflateInit(&from_input, Z_DEFAULT_COMPRESSION);
  if (ret < 0) {
    fprintf(stderr, "Failed compression init\n");
    exit(1);
  }

  from_server.zalloc = Z_NULL;
  from_server.zfree = Z_NULL;
  from_server.opaque = Z_NULL;
  ret = inflateInit(&from_server);
  if (ret < 0) {
    fprintf(stderr, "Failed decompression init\n");
    exit(1);
  }
}

int compression(int bytes_to_compress, char* buffer, char* compressedBuffer, int compressedBufferSize) {
  from_input.avail_in = bytes_to_compress;
  from_input.next_in = (Bytef *) buffer;
  from_input.avail_out = compressedBufferSize;
  from_input.next_out = (Bytef *) compressedBuffer;

  do {
    int ret = deflate(&from_input, Z_SYNC_FLUSH);
    if (ret == Z_STREAM_ERROR) {
      fprintf(stderr, "Compressing stream state inconsistent: %s\n", from_input.msg);
      exit(1);
    }
  } while (from_input.avail_in > 0);

  return compressedBufferSize - from_input.avail_out;
}

int decompression(int bytes_to_decompress, char* buffer, char* decompressedBuffer, int decompressedBufferSize) {
  from_server.avail_in = bytes_to_decompress;
  from_server.next_in = (Bytef *) buffer;
  from_server.avail_out = decompressedBufferSize;
  from_server.next_out = (Bytef *) decompressedBuffer;

  do {
    int ret = inflate(&from_server, Z_SYNC_FLUSH);
    if (ret == Z_STREAM_ERROR) {
      fprintf(stderr, "Decompressing stream state inconsistent: %s\n", from_server.msg);
      exit(1);
    }
  } while (from_server.avail_in > 0);

  return decompressedBufferSize - from_server.avail_out;
}

void run() {
  int ret;
  struct pollfd fds[2];

  fds[0].fd = 0; //standard input
  fds[0].events = POLLIN | POLLHUP | POLLERR;

  fds[1].fd = sockfd; //server output
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
      exit(1);
    }

    if (fds[0].revents & POLLIN) {
      size = read(fds[0].fd, &buf, 256); //keyboard input
      if (size < 0) {
        fprintf(stderr, "Failed to read from keyboard input");
        exit(1);
      }
      for(int i = 0; i < size; i++) {
        temp = buf[i];
        switch(temp) {
          case '\012':
          case '\015':
            write(1, &crlf, 2);
            if (!compress_flag)
              write(sockfd, &lf, 1);
            else
              compressbuf[i] = lf[0];
            break;
          default:
            write(1, &temp, 1);
            if (!compress_flag)
              write(sockfd, &temp, 1);
            else
              compressbuf[i] = temp;
            break;
        }
        if (log_flag && !compress_flag) {
          dprintf(logfile, "SENT %d bytes: ", 1);
          write(logfile, &temp, 1);
          write(logfile, &crlf[1], 1);
        }
      }
      if (compress_flag) {
        char tempbuf[256];
        size = compression(size, compressbuf, tempbuf, 256);
        write(sockfd, &tempbuf, size);
        if (log_flag) {
          dprintf(logfile, "SENT %d bytes: ", size);
          write(logfile, &tempbuf, size);
          write(logfile, &crlf[1], 1);
        }
      }

      memset(buf, 0, size);
    }

    if (fds[1].revents & POLLIN) {
      size = read(fds[1].fd, &buf, 256); //server output
      if (size < 0) {
        fprintf(stderr, "Failed to read server output");
        exit(1);
      }
      if (log_flag) {
        dprintf(logfile, "RECEIVED %d bytes: ", size);
        write(logfile, &buf, size);
        write(logfile, &crlf[1], 1);
      }
      if (compress_flag) {
        char tempbuf[256];
        size = decompression(size, buf, tempbuf, 256);
        write(1, &tempbuf, size);
      }
      else {
        write(1, &buf, size);
      }
      memset(buf, 0, size);
    }

    if ((POLLHUP | POLLERR) & fds[1].revents) {
      exit(0);
    }
  }
}

int main(int argc, char **argv) {
  //Process args
  int c;
  int port_flag 0;
  log_flag = 0;
  compress_flag = 0;
  while(1) {
    static struct option long_options[] = {
        {"port",  required_argument,  0,  'p'},
        {"log",   optional_argument,  0,  'l'},
        {"compress",    optional_argument, 0,  'c'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "p:l:", long_options, &option_index);

    if (c == -1)
      break;

    switch(c) {
      case 'p':
        port = atoi(optarg);
        port_flag = 1;
        break;
      case 'l':
        logfile = open(optarg, O_WRONLY | O_CREAT, 0666);
        if (logfile < 0) {
          fprintf(stderr, "Failed to create/open log file: %s\n", strerror(errno));
          exit(1);
        }
        log_flag = 1;
        break;
      case 'c':
        compress_flag = 1;
        break;
      default:
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "--port=port#  Port Number         Required\n");
        fprintf(stderr, "--log=logfile Log File            Optional\n");
        fprintf(stderr, "--compress    Enable Compression  Optional");
        exit(1);
    }
  }
  if (!port_flag) {
    fprintf(stderr, "Invalid arguments\n");
    fprintf(stderr, "--port=port#      Port Number     Required\n");
    fprintf(stderr, "--shell=<program> Shell program   Optional\n");
    fprintf(stderr, "--compress    Enable Compression  Optional");
    exit(1);
  }

  //Set terminal attributes
  setTerminal();

  //Connect to server
  connectToServer();

  if (compress_flag) {
    setupCompression();
  }

  run();
}
