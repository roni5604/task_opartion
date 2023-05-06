#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

void set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "Usage: %s [-c IP | -s] PORT\n", argv[0]);
    return 1;
  }

  struct addrinfo hints, *res;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  int server_mode = strcmp(argv[1], "-s") == 0;
  if (server_mode) {
    hints.ai_flags = AI_PASSIVE;
    getaddrinfo(NULL, argv[2], &hints, &res);
  } else {
    getaddrinfo(argv[2], argv[3], &hints, &res);
  }

  int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
  if (server_mode) {
    bind(sockfd, res->ai_addr, res->ai_addrlen);
    listen(sockfd, 1);
    fprintf(stderr, "Waiting for a connection...\n");
    int client_fd = accept(sockfd, NULL, NULL);
    close(sockfd);
    sockfd = client_fd;
  } else {
    connect(sockfd, res->ai_addr, res->ai_addrlen);
  }

  freeaddrinfo(res);
  set_non_blocking(sockfd);
  set_non_blocking(STDIN_FILENO);

  struct pollfd fds[2];
  fds[0].fd = STDIN_FILENO;
  fds[0].events = POLLIN;
  fds[1].fd = sockfd;
  fds[1].events = POLLIN;

  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  while (1) {
    poll(fds, 2, -1);

    if (fds[0].revents & POLLIN) {
      bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
      if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        write(sockfd, buffer, bytes_read);
      }
    }

    if (fds[1].revents & POLLIN) {
      bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
      if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Received: %s", buffer);
      } else if (bytes_read == 0) {
        printf("Connection closed\n");
        break;
      } else {
        perror("read");
        break;
      }
    }
  }

  close(sockfd);
  return 0;
}
