#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include "mraa.h"
#include <math.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <poll.h>

const int B = 4275;
const int R0 = 100000;
mraa_aio_context tempSensor;
mraa_gpio_context button;

int period;
char* scale;
int logflag;
int logfile;
int stop;

sig_atomic_t run_flag = 1;

void endProgram() {
  run_flag = 0;
  time_t timetemp = time(NULL);
  struct tm* curTime = localtime(&timetemp);
  dprintf(1, "%02d:%02d:%02d SHUTDOWN\n", curTime->tm_hour, curTime->tm_min, curTime->tm_sec);
  if (logflag) {
    dprintf(logfile, "%02d:%02d:%02d SHUTDOWN\n", curTime->tm_hour, curTime->tm_min, curTime->tm_sec);
  }
}

float getTemp() {
  int a = mraa_aio_read(tempSensor);
  float R = 1023.0/a-1.0;
  R = R0*R;
  float temperature = 1.0/(log(R/R0)/B+1/298.15)-273.15;

  if (strncmp(scale, "F", 1) == 0)
    return (temperature*1.8 + 32.0);
  return (temperature);
}

void handleCommand(char* buf) {
  if (strcmp(buf, "SCALE=F") == 0) {
    scale = "F";
    if (logflag) {
      dprintf(logfile, "SCALE=F\n");
    }
  }
  else if (strcmp(buf, "SCALE=C") == 0) {
    scale = "C";
    if (logflag) {
      dprintf(logfile, "SCALE=C\n");
    }
  }
  else if (strcmp(buf, "STOP") == 0) {
    stop = 1;
    if (logflag) {
      dprintf(logfile, "STOP\n");
    }
  }
  else if (strcmp(buf, "START") == 0) {
    stop = 0;
    if (logflag) {
      dprintf(logfile, "START\n");
    }
  }
  else if (strcmp(buf, "OFF") == 0) {
    if (logflag) {
      dprintf(logfile, "OFF\n");
    }
    endProgram();
  }
  else if (strncmp(buf, "PERIOD=", 7) == 0) {
    int newPeriod = atoi(buf + 7);
    if (newPeriod > 0) {
      if (logflag) {
        dprintf(logfile, "PERIOD=%d\n", newPeriod);
      }
      period = newPeriod;
    }
  }
  else if (logflag) {
    dprintf(logfile, "%s\n", buf);
  }
}

int main(int argc, char **argv) {

  //Process args
  int c;
  period = 1;
  scale = "F";
  logflag = 0;
  stop = 0;

  while(1) {
    static struct option long_options[] = {
        {"period",  optional_argument,  0,  'p'},
        {"scale",  optional_argument,  0,  's'},
        {"logfile",  optional_argument,  0,  'l'},
        {0, 0, 0, 0}
    };
    int option_index = 0;
    c = getopt_long(argc, argv, "p:s:l:", long_options, &option_index);
    if (c == -1)
        break;

    switch(c) {
      case 'l':
        logfile = open(optarg, O_WRONLY | O_CREAT, 0666);
        if (logfile < 0) {
           fprintf(stderr, "Failed to create/open log file: %s\n", strerror(errno));
           exit(1);
        }
        logflag = 1;
        break;
      case 'p':
        period = atoi(optarg);
        if (period <= 0) {
          fprintf(stderr, "Invalid temperature rate. Must be at least 1\n");
          exit(1);
        }
        break;
      case 's':
        scale = optarg;
        if (strncmp(scale, "F", 1) != 0 && strncmp(scale, "C", 1) != 0) {
          fprintf(stderr, "Invalid scale. Must be F or C. Default F\n");
          exit(1);
        }
        break;
      default:
        fprintf(stderr, "Unrecognized argument\n");
        fprintf(stderr, "--period=# Temperature output rate. Default 1\n");
        fprintf(stderr, "--scale=scale Farenheight (F) or Celcius (C). Default F\n");
        fprintf(stderr, "--logfile=filename Logfile Default stdout\n");
        exit(1);
    }
  }

  tempSensor = mraa_aio_init(1);
  button = mraa_gpio_init(60);
  mraa_gpio_dir(button, MRAA_GPIO_IN);

  mraa_gpio_isr(button, MRAA_GPIO_EDGE_RISING, &endProgram, NULL);

  struct pollfd fds[1];
  fds[0].fd = 0; //standard input
  fds[0].events = POLLIN | POLLHUP | POLLERR;
  int ret;
  char buf[128];
  char secbuf[128];
  memset(buf, 0, 128);
  memset(secbuf, 0, 128);
  int index = 0;
  int size;
  time_t presentTime;
  time_t oldTime;
  struct tm* curTime;

  while (run_flag) {
    presentTime = time(NULL);
    curTime = localtime(&presentTime);

    if (difftime(presentTime, oldTime) >= period && !stop) {
      float temp = getTemp();
      dprintf(1, "%02d:%02d:%02d %0.1f\n", curTime->tm_hour, curTime->tm_min, curTime->tm_sec, temp);
      if (logflag) {
        dprintf(logfile, "%02d:%02d:%02d %0.1f\n", curTime->tm_hour, curTime->tm_min, curTime->tm_sec, temp);
      }
      oldTime = presentTime;
    }

    ret = poll(fds, 1, 0);
    if (ret == -1) {
      fprintf(stderr, "Error while polling: %s\n", strerror(errno));
    }

    if (fds[0].revents & POLLIN) {
      size = read(fds[0].fd, &buf, 128);
      int i;
      for (i = 0; i < size; i++) {
        if (buf[i] == '\n') {
          handleCommand(secbuf);
          memset(secbuf, 0, 128);
          index = 0;
        }
        else {
          secbuf[index] = buf[i];
          index += 1;
        }
      }

    }
  }

  mraa_aio_close(tempSensor);
  mraa_gpio_close(button);

  return 0;
}
