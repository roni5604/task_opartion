#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/uds_stream_socket"
#define BUFFER_SIZE 1024
#define FILE_NAME_RECEIVE "received_file.txt"
#define EOF_MARKER "<<<EOF>>>"

void serverUdsDgram() {
    int server_fd, new_socket;
    struct sockaddr_un address, client_addr;
    int addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
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

    // Listen for connections
    if (listen(server_fd, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept a connection
    if ((new_socket = accept(server_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addr_len)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    FILE *fp = NULL;
    int total_bytes_received = 0;
    int eof_marker_size = strlen(EOF_MARKER);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    while (1) {
        int bytes_received = read(new_socket, buffer, BUFFER_SIZE);
        if (bytes_received <= 0) {
            if (bytes_received < 0) {
                perror("read");
            }
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
            total_bytes_received += (bytes_received - eof_marker_size);
            break; // Exit the loop
        }

        fwrite(buffer, 1, bytes_received, fp);
        total_bytes_received += bytes_received;
    }

    gettimeofday(&end, NULL);
    double time_taken = ((end.tv_sec - start.tv_sec) * 1e6 + (end.tv_usec - start.tv_usec)) * 1e-6;

    if (fp != NULL) {
        fclose(fp);
    }
    close(new_socket);
    close(server_fd);

    printf("File transfer complete. %d bytes received.\n", total_bytes_received);
    printf("Time taken: %.6lf seconds\n", time_taken);
}

void client(const char *file_name) {
    int sock;
    struct sockaddr_un serv_addr;
    char buffer[BUFFER_SIZE];

    // Create a socket
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sun_family = AF_UNIX;
    strncpy(serv_addr.sun_path, SOCKET_PATH, sizeof(serv_addr.sun_path) - 1);

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    struct stat file_stat;
    if (fstat(fd, &file_stat) < 0) {
        perror("fstat");
        exit(EXIT_FAILURE);
    }

    void *file_data = mmap(NULL, file_stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (file_data == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    ssize_t bytes_sent = 0;
    while (bytes_sent < file_stat.st_size) {
        ssize_t bytes_to_send = (file_stat.st_size - bytes_sent > BUFFER_SIZE) ? BUFFER_SIZE : file_stat.st_size - bytes_sent;
        if (write(sock, file_data + bytes_sent, bytes_to_send) != bytes_to_send) {
            perror("write");
            break;
        }
        bytes_sent += bytes_to_send;
    }

    // Send EOF marker
    strncpy(buffer, EOF_MARKER, strlen(EOF_MARKER));
    if (write(sock, buffer, strlen(EOF_MARKER)) != strlen(EOF_MARKER)) {
        perror("write");
    }

    munmap(file_data, file_stat.st_size);
    close(fd);
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [s|c] [filename (only for client)]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "s") == 0) {
        serverUdsDgram();
    } else if (strcmp(argv[1], "c") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s c [filename]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
        client(argv[2]);
    } else {
        fprintf(stderr, "Invalid option. Use 's' for server or 'c' for client.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}
