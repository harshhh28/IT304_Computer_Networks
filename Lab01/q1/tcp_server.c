// Server side C program (TCP)

#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>

#define PORT 8080
#define MAX_QA_PAIRS 10

typedef struct {
    char ques[1024];
    char ans[1024];
} QAPair;

QAPair qa_pairs[MAX_QA_PAIRS];
int qa_count = 0;

void load_qas_from_file(const char *filename) {
    
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Failed to open Q&A file");
        exit(EXIT_FAILURE);
    }

    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        if (qa_count >= MAX_QA_PAIRS) break;
        char *token = strtok(line, ";");
        if (token != NULL) {
            strncpy(qa_pairs[qa_count].ques, token, sizeof(qa_pairs[qa_count].ques));
            token = strtok(NULL, ";\n");
            if (token != NULL) {
                strncpy(qa_pairs[qa_count].ans, token, sizeof(qa_pairs[qa_count].ans));
                qa_count++;
            }
        }
    }

    fclose(file);
}

int main(int argc, char const *argv[]) {

    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Load Q&A pairs from the file
    load_qas_from_file("qa.txt");

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port 8080 - For address reuse
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET; // match the socket() call
    address.sin_addr.s_addr = INADDR_ANY; // bind to any local address
    address.sin_port = htons(PORT); // Specify port to listen on forcefully attaching socket to the port 8080
    
    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    // Accept
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    while (1) {
        valread = read(new_socket, buffer, 1024);
        if (valread < 0) {
            perror("read");
            break;
        }

        printf("Client selected option: %s\n", buffer);
        int option = atoi(buffer);

        if (option > 0 && option <= qa_count) {
            send(new_socket, qa_pairs[option - 1].ans, strlen(qa_pairs[option - 1].ans), 0);
        } else {
            char *invalid_response = "Server: Invalid option!\n";
            send(new_socket, invalid_response, strlen(invalid_response), 0);
        }

        memset(buffer, 0, sizeof(buffer));
    }

    close(new_socket);
    close(server_fd);

    return 0;
}

