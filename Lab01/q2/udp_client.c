// Client side C program (UDP)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>

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
    struct sockaddr_in servaddr;
    char buffer[MAXLINE];
    char message[MAXLINE];
    uint32_t checksum;

    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    while(1) {
        printf("Client: ");
        fgets(message, MAXLINE, stdin);
        message[strcspn(message, "\n")] = 0;

        // Compute checksum
        checksum = compute_checksum(message, strlen(message));

        // Send message with checksum
        char packet[MAXLINE + sizeof(checksum)];
        memcpy(packet, &checksum, sizeof(checksum));
        memcpy(packet + sizeof(checksum), message, strlen(message));
        sendto(sockfd, packet, sizeof(checksum) + strlen(message), 0, (struct sockaddr*)&servaddr, sizeof(servaddr));

        // Receive response
        socklen_t len;
        int n = recvfrom(sockfd, buffer, MAXLINE, 0, (struct sockaddr*)&servaddr, &len);
        buffer[n] = '\0';

        // Extract checksum from response
        uint32_t response_checksum;
        memcpy(&response_checksum, buffer, sizeof(response_checksum));
        char *response_message = buffer + sizeof(response_checksum);

        // Verify checksum
        if (compute_checksum(response_message, strlen(response_message)) != response_checksum) {
            printf("Received corrupted message\n");
        }
        else {
            printf("Server: %s\n", response_message);
        }
    }

    close(sockfd);
    return 0;
}