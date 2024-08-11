// Server side C program (UDP)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAXLINE 1024

uint32_t compute_checksum(const char *data, size_t length) {
    uint32_t checksum = 0;
    for (size_t i = 0; i < length; ++i) {
        checksum += (uint8_t)data[i];
    }
    return checksum;
}

int main() {
    
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[MAXLINE];
    uint32_t checksum;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind
    if (bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1) {
        socklen_t len = sizeof(cliaddr);

        if (recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&cliaddr, &len) < 0) {
            perror("Receive failed");
            continue;
        }

        // Extract checksum from received packet
        memcpy(&checksum, buffer, sizeof(checksum));
        char *received_message = buffer + sizeof(checksum);

        // Exit if client sends "q"
        if (strcmp(received_message, "q") == 0) {
            printf("Client has disconnected.\n");
            break;
        }

        // Verify checksum
        if (compute_checksum(received_message, strlen(received_message)) != checksum) {
            printf("Received corrupted message\n");
            char *error_message = "Error! Corrupted packet!";
            checksum = compute_checksum(error_message, strlen(error_message));
            char response[MAXLINE + sizeof(checksum)];
            memcpy(response, &checksum, sizeof(checksum));
            memcpy(response + sizeof(checksum), error_message, strlen(error_message));
            sendto(sockfd, response, sizeof(checksum) + strlen(error_message), 0, (struct sockaddr*)&cliaddr, len);
        }
        else {
            printf("Received message: %s\n", received_message);
            char *ack_message = "Message received correctly!";
            checksum = compute_checksum(ack_message, strlen(ack_message));
            char response[MAXLINE + sizeof(checksum)];
            memcpy(response, &checksum, sizeof(checksum));
            memcpy(response + sizeof(checksum), ack_message, strlen(ack_message));
            sendto(sockfd, response, sizeof(checksum) + strlen(ack_message), 0, (struct sockaddr*)&cliaddr, len);
        }
    }

    close(sockfd);
    return 0;
}
