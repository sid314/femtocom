#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
struct termios keyb_backup;
void restore_terminal(void) {
  tcsetattr(STDIN_FILENO, TCSANOW, &keyb_backup);
  printf("\r\nTerminal restored. Goodbye!\r\n");
}
int main(int argc, char *argv[]) {
  char *portname = NULL;
  char *baudrate = NULL;
  int opt;
  while ((opt = getopt(argc, argv, "p:b:")) != -1) {
    switch (opt) {
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
  printf("   Port: %s\n", portname);
  printf("   Baud: %s\n", baudrate);

  int fd = open(portname, O_RDWR | O_NOCTTY);
  if (fd < 0) {
    perror("Failed to open serial port");
    return EXIT_FAILURE;
  }
  struct termios keyb;
  struct termios port;
  if (tcgetattr(STDIN_FILENO, &keyb) < 0) {
    perror("Warning: Failed to get stdin attributes");
  }
  keyb_backup = keyb;
  atexit(restore_terminal);
  if (tcgetattr(fd, &port) < 0) {
    perror("Warning: Failed to get serial port attributes");
  }
  speed_t speed;
  switch (atoi(baudrate)) {
  case 115200:
    speed = B115200;
    break;
  default:
    printf("Unsupported speed\n");
    return EXIT_FAILURE;
  }
  cfsetispeed(&port, speed);
  cfsetospeed(&port, speed);
  cfmakeraw(&port);
  cfmakeraw(&keyb);
  tcsetattr(STDIN_FILENO, TCSANOW, &keyb);
  tcsetattr(fd, TCSANOW, &port);
  fsync(STDOUT_FILENO);
  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = fd;
  fds[1].events = POLLIN;
  while (1) {
    int ret = poll(fds, 2, -1);
    if (ret < 0) {
      break;
    }
    if (fds[0].revents & POLLIN) {
      char tx_char;
      if (read(STDIN_FILENO, &tx_char, 1) > 0) {
        if (tx_char == 0x18) {
          break;
        }
        write(fd, &tx_char, 1);
      }
    }
    if (fds[1].revents & POLLIN) {
      char rx_char;
      if (read(fd, &rx_char, 1) > 0) {
        write(STDOUT_FILENO, &rx_char, 1);
        fsync(STDOUT_FILENO);
      }
    }
  }
  close(fd);
  restore_terminal();
  return EXIT_SUCCESS;
}
