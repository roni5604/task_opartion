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

typedef enum
{
  SERVER,
  CLIENT
} Role;

void set_non_blocking(int fd)
{ // this function add the O_NONBLOCK flag to the file descriptor
  int flags = fcntl(fd, F_GETFL, 0);
  fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int create_socket(struct addrinfo *res)
{
  return socket(res->ai_family, res->ai_socktype, res->ai_protocol);
}

void setup_server(struct addrinfo *res, int *sockfd)
{
  bind(*sockfd, res->ai_addr, res->ai_addrlen);
  listen(*sockfd, 1);
  fprintf(stderr, "Waiting for a connection...\n");
  int client_fd = accept(*sockfd, NULL, NULL);
  printf("Connection established lets start talk with the client\n");
  close(*sockfd);
  *sockfd = client_fd;
}

void setup_client(struct addrinfo *res, int *sockfd)
{
  connect(*sockfd, res->ai_addr, res->ai_addrlen);
  printf("Connection established lets start talk with the server\n");
}

void poll_fds(struct pollfd *fds)
{
  poll(fds, 2, -1);
}

void handle_io(struct pollfd *fds, Role role, int sockfd)
{
  char buffer[BUFFER_SIZE];
  ssize_t bytes_read;

  if (fds[0].revents & POLLIN) // if there is data to read from stdin
  {
    bytes_read = read(STDIN_FILENO, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0)
    {
      buffer[bytes_read] = '\0';
      write(sockfd, buffer, bytes_read);
      if (strcmp(buffer, "exit\n") == 0)
      {
        printf("Connection closed\n");
        close(sockfd);
        return;
      }
    }
  }

  if (fds[1].revents & POLLIN) // id there is data to read from the socket
  {
    bytes_read = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0)
    {
      buffer[bytes_read] = '\0';
      if (role == SERVER)
      {
        printf("Client: %s", buffer);
      }
      else
      {
        printf("Server: %s", buffer);
      }
      if (strcmp(buffer, "exit\n") == 0)
      {
        printf("Connection closed\n");
        close(sockfd);
        return; // if the message is exit then we break the loop
      }
    }
    else if (bytes_read == 0)
    {
      printf("Connection closed\n");
    }
    else
    {
      perror("read");
    }
  }
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    fprintf(stderr, "Usage: %s [-c IP | -s] PORT\n", argv[0]);
    return 1;
  }
  int client_mode = -1;
  int server_mode = -1;
  int quiet_mode = -1;
  int performance = -1;
  int opt;
  for (int i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], "-p") == 0)
    {
      performance = 1;
    }
    else if (strcmp(argv[i], "-q") == 0)
    {
      quiet_mode = 1;
    }
    else if (strcmp(argv[i], "-c") == 0)
    {
      client_mode = 1;
    }
    else if (strcmp(argv[i], "-s") == 0)
    {
      server_mode = 1;
    }
  }

  if (client_mode == -1 && server_mode == -1)
  {
    fprintf(stderr, "Usage: %s [-c IP | -s] PORT\n", argv[0]);
    return 1;
  }
  if (performance == 1)
  {
    printf("Performance mode is on\n");

    if (client_mode == 1)
    {
      printf("client mode is on\n");
      if (argc != 7)
      {
        fprintf(stderr, "Usage: %s -c IP PORT -p <param> <type> \n", argv[0]);
        return 1;
      }

      // find the param and type here
      char *param = NULL;
      char *type = NULL;
      int i,errorP=0;
      for (i = 4; i < 7; ++i)
      {
        if ((strcmp(argv[i], "tcp") == 0) || (strcmp(argv[i], "udp") == 0) || (strcmp(argv[i], "dgram") == 0) || (strcmp(argv[i], "stream") == 0))
        {
          param = (char *)malloc(strlen(argv[i]) + 1); // allocate memory for param
          if (param == NULL)
          {
            printf("Memory allocation error.\n");
            exit(1);
          }
          strcpy(param, argv[i]);
          printf("param = %s\n", param);
        }
        else if ((strcmp(argv[i], "ipv4") == 0) || (strcmp(argv[i], "ipv6") == 0) || (strcmp(argv[i], "mmap") == 0) || (strcmp(argv[i], "pipe") == 0) || (strcmp(argv[i], "uds") == 0))
        {
          type = (char *)malloc(strlen(argv[i]) + 1); // allocate memory for type
          if (type == NULL)
          {
            printf("Memory allocation error.\n");
            exit(1);
          }
          strcpy(type, argv[i]);
          printf("type = %s\n", type);
        }
        else if (strchr(argv[i], '.') != NULL) // th case of the filename (must contain a dot)
        {
       param = (char *)malloc(strlen(argv[i]) + 1); // allocate memory for param
          if (param == NULL)
          {
            printf("Memory allocation error.\n");
            exit(1);
          }
          strcpy(param, argv[i]);
          printf("param = %s\n", param);
        }
        else if ((strcmp(argv[i], "-p") != 0))
        {
          fprintf(stderr, "Usage: %s -c IP PORT -p <param> <type> \n", argv[0]);
          return 1;
        }
        if (strcmp(argv[i], "-p") == 0)
        {
          errorP++;
          if (errorP>1)
          {
            fprintf(stderr, "Usage: %s -c IP PORT -p <param> <type> \n", argv[0]);
            return 1;
          }
        }
      
        
      
      }
      if (param == NULL || type == NULL)
      {
        fprintf(stderr, "Usage: %s -c IP PORT -p <param> <type> \n", argv[0]);
        return 1;
      }
     
    }
    else if (server_mode == 1)
    {
      printf("server mode is on\n");
      if (argc != 4 && argc != 5)
      {
        printf("argc = %d\n", argc);
        fprintf(stderr, "Usage: %s -s PORT -p -q \n", argv[0]);
        return 1;
      }
      if (argc == 5 && performance == 1 && quiet_mode == 1)
      {
        printf("quiet mode is on\n");
      }
    }
    else
    {
      fprintf(stderr, "Usage: %s [-c IP | -s] PORT\n", argv[0]);
      return 1;
    }
  }
  else
  {
    // part 1
    printf("Performance mode is off\n");

    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    Role role = server_mode ? SERVER : CLIENT;

    if (server_mode == 1)
    {
      if (argc != 3)
      {
        fprintf(stderr, "Usage: %s -s PORT\n", argv[0]);
        return 1;
      }
      hints.ai_flags = AI_PASSIVE;
      getaddrinfo(NULL, argv[2], &hints, &res);
    }
    else if (client_mode == 1)
    {
      if (argc != 4)
      {
        fprintf(stderr, "Usage: %s -c IP PORT\n", argv[0]);
        return 1;
      }
      getaddrinfo(argv[2], argv[3], &hints, &res);
    }
    else
    {
      fprintf(stderr, "Usage: %s [-c IP | -s] PORT\n", argv[0]);
      return 1;
    }

    int sockfd = create_socket(res);
    if (server_mode == 1)
    {
      setup_server(res, &sockfd);
    }
    else if (client_mode == 1)
    {
      setup_client(res, &sockfd);
    }

    freeaddrinfo(res);
    set_non_blocking(sockfd);
    set_non_blocking(STDIN_FILENO);

    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sockfd;
    fds[1].events = POLLIN;

    while (1)
    {
      poll_fds(fds);

      handle_io(fds, role, sockfd);

      if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
      { // if there is an error in the socket then we break the loop
        break;
      }
    }

    close(sockfd);
    return 0;
  }
}