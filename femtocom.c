#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>
struct termios keyb_backup;
void restore_terminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &keyb_backup);
  printf("\nTerminal restored. Goodbye!\n");
}
int main(int argc, char *argv[]) {
  char *portname = NULL;
  char *baudrate = NULL;
  char c;
  while ((c = getopt(argc, argv, "p:b:")) != -1) {
    switch (c) {
    case 'p':
      portname = optarg;
      break;
    case 'b':
      baudrate = optarg;
      break;
    }
  }
  if (portname == NULL || baudrate == NULL) {
    printf("Usage: %s -p <port> -b <baud>\n", argv[0]);
    return EXIT_FAILURE;
  }
  printf("Baud: %s\n", baudrate);
  printf("Rort: %s\n", portname);
  int fd = open(portname, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror("Failed to open serial port");
    return EXIT_FAILURE;
  }
  printf("Port %s opened at %s baud with file descriptor %d\n", portname,
         baudrate, fd);
  struct termios keyb;
  struct termios port;
  tcgetattr(STDIN_FILENO, &keyb);
  tcgetattr(STDIN_FILENO, &keyb_backup);
  atexit(restore_terminal);
  tcgetattr(fd, &port);
  speed_t speed;
  switch (atoi(baudrate)) {
  case 115200:
    speed = B115200;
    break;
  default:
    printf("Unsupported speed");
    return EXIT_FAILURE;
  }
  cfsetispeed(&port, speed);
  cfsetospeed(&port, speed);
  cfmakeraw(&port);
  cfmakeraw(&keyb);
  tcsetattr(STDIN_FILENO, TCSANOW, &keyb);
  tcsetattr(fd, TCSANOW, &port);
  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = fd;
  fds[1].events = POLLIN;
  while (1) {
    poll(fds, 2, -1);
    if (fds[0].revents & POLLIN) {
      char c;
      read(STDIN_FILENO, &c, 1);
      if (c == 0x11) {
        break;
      }
      write(fd, &c, 1);
    }
    if (fds[1].revents & POLLIN) {
      char c;
      read(fd, &c, 1);
      write(STDOUT_FILENO, &c, 1);
    }
  }
  close(fd);
  return EXIT_SUCCESS;
}
