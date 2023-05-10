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
#define EOF_MARKER "<<<EOF>>>"

void serverUdpIpv4(uint16_t port)
{
    int server_fd;
    struct sockaddr_in address, client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", port);

    struct pollfd fds[1];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    FILE *fp = fopen(FILE_NAME_RECEIVE, "wb");
    if (fp == NULL)
    {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    int total_bytes_received = 0;
    int poll_ret;
    int eof_marker_size = strlen(EOF_MARKER);

    while (1)
    {
        poll_ret = poll(fds, 1, -1); // No timeout, wait indefinitely

        if (poll_ret > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                int bytes_received = recvfrom(server_fd, buffer, BUFFER_SIZE, 0,
                                              (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
                if (bytes_received <= 0)
                {
                    perror("recvfrom");
                    break;
                }

                // Check for EOF marker
                if (bytes_received >= eof_marker_size && memcmp(buffer + bytes_received - eof_marker_size, EOF_MARKER, eof_marker_size) == 0)
                {
                    fwrite(buffer, 1, bytes_received - eof_marker_size, fp); // Write remaining data before EOF marker
                    break;                                                   // Exit the loop
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
    close(server_fd);

    printf("File transfer completed. Received %d bytes.\n", total_bytes_received);
}

void clientUdpIpv4(uint16_t port, char * ipUdpIpv4)
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // Convert IPv4 address from text to binary form
    if (inet_pton(AF_INET, ipUdpIpv4, &serv_addr.sin_addr) <= 0)
    {
        perror("inet_pton");
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
        sendto(sock, buffer, bytes_read, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }

    // Send EOF marker
    int eof_marker_size = strlen(EOF_MARKER);
    sendto(sock, EOF_MARKER, eof_marker_size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

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
        serverUdpIpv4(8080);
    }
    else if (strcmp(argv[1], "c") == 0)
    {
        clientUdpIpv4(8080,"127.0.0.1");
    }
    else
    {
        fprintf(stderr, "Invalid argument: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
