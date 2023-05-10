#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define FILE_NAME_SEND "file_to_send.txt"
#define FILE_NAME_RECEIVE "received_file.txt"

void serverTcpIpv6(uint16_t port)
{
    int server_fd, new_socket;
    struct sockaddr_in6 address;
    int addr_len = sizeof(address);
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((server_fd = socket(AF_INET6, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    address.sin6_family = AF_INET6;
    address.sin6_addr = in6addr_any;
    address.sin6_port = htons(port);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", port);

    // Accept a client connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addr_len)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[1];
    fds[0].fd = new_socket;
    fds[0].events = POLLIN;

    FILE *fp = fopen(FILE_NAME_RECEIVE, "wb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int total_bytes_received = 0;
    int poll_ret;

    while (total_bytes_received < 1024 * 1024)
    {
        poll_ret = poll(fds, 1, -1); // No timeout, wait indefinitely

        if (poll_ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                int bytes_received = recv(new_socket, buffer, BUFFER_SIZE, 0);
                if (bytes_received <= 0)
                {
                    perror("recv");
                    break;
                }

                fwrite(buffer, 1, bytes_received, fp);
                total_bytes_received += bytes_received;
            }
        }
        else
        {
            perror("poll");
            break;
        }
    }

    fclose(fp);
    close(new_socket);
    close(server_fd);

    printf("File transfer completed. Received %d bytes.\n", total_bytes_received);
}

void clientTcpIpv6(uint16_t port, char * ip_ipv6)
{
    int sock = 0;
    struct sockaddr_in6 serv_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin6_family = AF_INET6;
    serv_addr.sin6_port = htons(port);

    // Convert IPv6 address from text to binary form
    if (inet_pton(AF_INET6, ip_ipv6, &serv_addr.sin6_addr) <= 0)
    {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    FILE *fp = fopen(FILE_NAME_SEND, "rb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
    {
        send(sock, buffer, bytes_read, 0);
    }

    fclose(fp);
    close(sock);

    printf("File transfer completed.\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [s|c]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "s") == 0)
    {
        serverTcpIpv6(8080);
    }
    else if (strcmp(argv[1], "c") == 0)
    {
        clientTcpIpv6(8080,"::1");
    }
    else
    {
        fprintf(stderr, "Invalid argument: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
