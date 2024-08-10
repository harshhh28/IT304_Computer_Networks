// Serverside C/C++ program to demonstrate Socket programming 
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#define PORT 8080 
int main(int argc, char const *argv[]) { 
int server_fd, new_socket, valread; 
struct sockaddr_in address; 
int opt = 1; 
int addrlen = sizeof(address); 
char buffer[1024]; 
char *hello = "Hello from server"; 
// Creating socket file descriptor 
if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) { 
perror("socket failed"); 
exit(EXIT_FAILURE); 
} 
// Forcefully attaching socket to the port 8080 - For address reuse
if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) { 
perror("setsockopt"); 
exit(EXIT_FAILURE); 
} 
address.sin_family = AF_INET; // match the socket() call
address.sin_addr.s_addr = INADDR_ANY; // bind to any local address address.sin_port = htons(PORT); // specify port to listen on 
// Forcefully attaching socket to the port 8080 
if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { perror("bind failed"); 
exit(EXIT_FAILURE); 
} 
if (listen(server_fd, 3) < 0) { 
perror("listen"); 
exit(EXIT_FAILURE); 
}
if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) { perror("accept"); 
exit(EXIT_FAILURE); 
} 
while (1) { 
valread = read(new_socket, buffer, 1024); 
printf("Client: %s\n", buffer); 
memset(buffer, 0, 1024); 
send(new_socket, hello, strlen(hello), 0); 
printf("Hello message sent\n"); 
} 
return 0; 
} 
