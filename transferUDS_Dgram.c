#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/uds_dgram_socket"
#define BUFFER_SIZE 1024
#define FILE_NAME_SEND "file_to_send.txt"
#define FILE_NAME_RECEIVE "received_file.txt"
#define EOF_MARKER "<<<EOF>>>"

void serverUdsDgram() {
    int server_fd;
    struct sockaddr_un address, client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((server_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    unlink(SOCKET_PATH);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    struct pollfd fds[1];
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    FILE *fp = NULL;
    int total_bytes_received = 0;
    int poll_ret;
    int eof_marker_size = strlen(EOF_MARKER);

    while (1) {
        poll_ret = poll(fds, 1, -1); // No timeout, wait indefinitely

        if (poll_ret > 0) {
            if (fds[0].revents & POLLIN) {
                int bytes_received = recvfrom(server_fd, buffer, BUFFER_SIZE, 0,
                                              (struct sockaddr *)&client_addr, (socklen_t *)&addr_len);
                if (bytes_received <= 0) {
                    perror("recvfrom");
                    break;
                }

                if (fp == NULL) {
                    fp = fopen(FILE_NAME_RECEIVE, "wb");
                    if (fp == NULL) {
                        perror("fopen");
                        exit(EXIT_FAILURE);
                    }
                }

                // Check for EOF marker
                if (bytes_received >= eof_marker_size && memcmp(buffer + bytes_received - eof_marker_size, EOF_MARKER, eof_marker_size) == 0) {
                    fwrite(buffer, 1, bytes_received - eof_marker_size, fp); // Write remaining data before EOF marker
                    break; // Exit the loop
                }

                fwrite(buffer, 1, bytes_received, fp);
                total_bytes_received += bytes_received;
            }
        } else {
            perror("poll");
            break;
        }
    }

    if (fp != NULL) {
        fclose(fp);
    }
    close(server_fd);

    printf("File transfer complete. %d bytes received.\n", total_bytes_received);
}


void clientUdsDgram() {
    int sock = 0;
    struct sockaddr_un serv_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((sock = socket(AF_UNIX, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKET_PATH, sizeof(serv_addr.sun_path) - 1);

    FILE *fp = fopen(FILE_NAME_SEND, "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        sendto(sock, buffer, bytes_read, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    }

    // Send EOF marker
    int eof_marker_size = strlen(EOF_MARKER);
    sendto(sock, EOF_MARKER, eof_marker_size, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    gettimeofday(&end_time, NULL);

    fclose(fp);
    close(sock);

    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    printf("File transfer completed in %.2f milliseconds.\n", elapsed_time);
}

int main(int argc, char *argv[]) {
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [s|c]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "s") == 0)
    {
        serverUdsDgram();
    }
    else if (strcmp(argv[1], "c") == 0)
    {
        clientUdsDgram();
    }
    else
    {
        fprintf(stderr, "Invalid argument: %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    return 0;
}
