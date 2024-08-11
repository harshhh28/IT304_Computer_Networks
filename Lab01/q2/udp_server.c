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
    for(size_t i = 0; i < length; ++i) {
        checksum += (uint8_t)data[i];
    }
    return checksum;
}

int main() {
    
    int sockfd;
    struct sockaddr_in serv_addr, cli_addr;
    char buffer[MAXLINE];
    uint32_t checksum;

    // Creating a UDP socket
    if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&erv, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Binds the socket to given addr and port
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    while(1) {
        socklen_t len = sizeof(cli_addr);
        int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&cli_addr, &len);

        if(n < 0) {
            perror("Receive failed");
            continue;
        }

        // Extract checksum from received packet
        memcpy(&checksum, buffer, sizeof(checksum));
        char *received_message = buffer + sizeof(checksum);

        // Verify checksum
        if(compute_checksum(received_message, strlen(received_message)) != checksum) {
            printf("Received corrupted message\n");
            char *error_message = "Error! Corrupted packet!";
            checksum = compute_checksum(error_message, strlen(error_message));
            char response[MAXLINE + sizeof(checksum)];
            memcpy(response, &checksum, sizeof(checksum));
            memcpy(response + sizeof(checksum), error_message, strlen(error_message));
            sendto(sockfd, response, sizeof(checksum) + strlen(error_message), 0, (struct sockaddr*)&cli_addr, len);
        }
        else {
            printf("Received message: %s\n", received_message);
            char *ack_message = "Message received correctly!";
            checksum = compute_checksum(ack_message, strlen(ack_message));
            char response[MAXLINE + sizeof(checksum)];
            memcpy(response, &checksum, sizeof(checksum));
            memcpy(response + sizeof(checksum), ack_message, strlen(ack_message));
            sendto(sockfd, response, sizeof(checksum) + strlen(ack_message), 0, (struct sockaddr*)&cli_addr, len);
        }
    }

    close(sockfd);
    return 0;
}
