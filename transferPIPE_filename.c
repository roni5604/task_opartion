#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define FILE_NAME_RECEIVE "received_file.dat"
#define EOF_MARKER "<<<EOF>>>"

void child_process(const char *file_name, int pipe_fd[2]) {
    close(pipe_fd[0]); // Close read end of the pipe

    FILE *fp = fopen(file_name, "rb");
    if (fp == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    char buffer[BUFFER_SIZE];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
        if (write(pipe_fd[1], buffer, bytes_read) != bytes_read) {
            perror("write");
            break;
        }
    }

    // Send EOF marker
    strncpy(buffer, EOF_MARKER, strlen(EOF_MARKER));
    if (write(pipe_fd[1], buffer, strlen(EOF_MARKER)) != strlen(EOF_MARKER)) {
        perror("write");
    }

    fclose(fp);
    close(pipe_fd[1]); // Close write end of the pipe
}

void parent_process(int pipe_fd[2]) {
    close(pipe_fd[1]); // Close write end of the pipe

    FILE *fp = NULL;
    char buffer[BUFFER_SIZE];
    int total_bytes_received = 0;
    int poll_ret;
    int eof_marker_size = strlen(EOF_MARKER);

    struct pollfd fds[1];
    fds[0].fd = pipe_fd[0];
    fds[0].events = POLLIN;

    struct timeval start, end;
    gettimeofday(&start, NULL);

    while (1) {
        poll_ret = poll(fds, 1, -1); // No timeout, wait indefinitely

        if (poll_ret < 0) {
            perror("poll");
            break;
        }

        int bytes_received = read(pipe_fd[0], buffer, BUFFER_SIZE);
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
    close(pipe_fd[0]); // Close read end of the pipe

    printf("File transfer complete. %d bytes received.\n", total_bytes_received);
    printf("Time taken: %.6lf seconds\n", time_taken);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) < 0) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        child_process(argv[1], pipe_fd);
    } else { // Parent process
        parent_process(pipe_fd);
        wait(NULL); // Wait for child process to finish
    }

    return 0;
}
