// Client side C program (TCP)

#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define PORT 8080
#define MAX_QA_PAIRS 10

int main(int argc, char const *argv[]) {

    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char msg[1024];
    char buffer[1024] = {0};
    char *questions[MAX_QA_PAIRS];
    int question_count = 0;

    // Reading the questions from the file
    FILE *file = fopen("qa.txt", "r");
    if(file == NULL) {
        perror("Failed to open Q&A file");
        return -1;
    }

    char line[1024];
    while(fgets(line, sizeof(line), file) != NULL) {
        if(question_count >= MAX_QA_PAIRS) break;

        char *delimiter_pos = strchr(line, ';');
        if(delimiter_pos != NULL) {
            *delimiter_pos = '\0';
        }

        questions[question_count] = strdup(line);
        question_count++;
    }

    fclose(file);

    // Socket connection
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/Address not supported\n");
        return -1;
    }
    
    // Check connection
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    // Display the questions
    printf("Select a question by typing the corresponding number:\n\n");
    for(int i = 0; i < question_count; i++) {
        printf("%d. %s\n", i + 1, questions[i]);
    }
    printf("Type 'q' to quit.\n");
    
    while(1) {
        printf("\nClient: ");
        
        fgets(msg, 1024, stdin);
        msg[strcspn(msg, "\n")] = 0;

        if(strcmp(msg, "q") == 0) {
            send(sock, msg, strlen(msg), 0);
            close(sock);
            break;
        }

        send(sock, msg, strlen(msg), 0);

        valread = read(sock, buffer, 1024);
        printf("%s\n", buffer);
        
        memset(buffer, 0, sizeof(buffer));
    }

    for(int i = 0; i < question_count; i++) {
        free(questions[i]);
    }

    return 0;
}