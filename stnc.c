#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_MSG_LEN 1024

void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, n;
    struct sockaddr_in serv_addr, cli_addr;
    socklen_t clilen;
    char buffer[MAX_MSG_LEN];

    if (argc < 3) {
        fprintf(stderr, "Usage: %s [-s PORT] or %s [-c IP PORT]\n", argv[0], argv[0]);
        exit(1);
    }

    if (strcmp(argv[1], "-s") == 0) {
        // server mode
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));
        portno = atoi(argv[2]);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR on binding");
        listen(sockfd, 5);
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("ERROR on accept");

        while (1) {
            bzero(buffer, MAX_MSG_LEN);
            n = read(newsockfd, buffer, MAX_MSG_LEN - 1);
            if (n < 0)
                error("ERROR reading from socket");
            printf("Client: %s", buffer);
            printf("Server: ");
            fgets(buffer, MAX_MSG_LEN - 1, stdin);
            n = write(newsockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
        }
        close(newsockfd);
        close(sockfd);
    } else if (strcmp(argv[1], "-c") == 0) {
        // client mode
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
            error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));
        portno = atoi(argv[3]);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(argv[2]);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error("ERROR connecting");

        while (1) {
            printf("Client: ");
            bzero(buffer, MAX_MSG_LEN);
            fgets(buffer, MAX_MSG_LEN - 1, stdin);
            n = write(sockfd, buffer, strlen(buffer));
            if (n < 0)
                error("ERROR writing to socket");
            bzero(buffer, MAX_MSG_LEN);
            n = read(sockfd, buffer, MAX_MSG_LEN - 1);
            if (n < 0)
                error("ERROR reading from socket");
            printf("Server: %s", buffer);
        }
        close(sockfd);
    } else {
        fprintf(stderr, "Usage: %s [-s PORT] or %s [-c IP PORT]\n", argv[0], argv[0]);
    }
    return 0;
}
