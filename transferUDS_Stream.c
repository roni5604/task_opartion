#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/uds_stream_socket"
#define BUFFER_SIZE 1024
#define FILE_NAME_SEND "file_to_send.txt"
#define FILE_NAME_RECEIVE "received_file.txt"
#define EOF_MARKER "<<<EOF>>>"

void serverUdsStream() {
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
    int poll_ret;
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

void clientUdsStream() {
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

    FILE *fp = fopen(FILE_NAME_SEND, "rb");
    if (fp == NULL) {
                perror("fopen");
        exit(EXIT_FAILURE);
    }

    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        if (write(sock, buffer, bytes_read) != bytes_read) {
            perror("write");
            break;
        }
    }

    // Send EOF marker
    strncpy(buffer, EOF_MARKER, strlen(EOF_MARKER));
    if (write(sock, buffer, strlen(EOF_MARKER)) != strlen(EOF_MARKER)) {
        perror("write");
    }

    fclose(fp);
    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s [s|c]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "s") == 0) {
        serverUdsDgram();
    } else if (strcmp(argv[1], "c") == 0) {
        clientUdsDgram();
    } else {
        fprintf(stderr, "Invalid option. Use 's' for server or 'c' for client.\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

       
