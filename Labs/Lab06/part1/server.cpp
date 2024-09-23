#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <dirent.h>
#include <thread>
#include <chrono>

const int CONTROL_PORT = 8080;
const int DATA_PORT = 8089;
const int BUFFER_SIZE = 1024;

void handleClient(int clientSocket) {
    char buffer[BUFFER_SIZE];
    std::string response;

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        std::string command(buffer);
        if (command.substr(0, 3) == "DIR") {
            DIR *dir;
            struct dirent *ent;
            if ((dir = opendir(".")) != NULL) {
                while ((ent = readdir(dir)) != NULL) {
                    response += ent->d_name;
                    response += "\n";
                }
                closedir(dir);
            } else {
                response = "Error: Could not open directory";
            }
        } else if (command.substr(0, 3) == "GET") {
            std::string filename = command.substr(4);
            std::ifstream file(filename, std::ios::binary);
            if (file) {
                response = "Ready to transfer";
                send(clientSocket, response.c_str(), response.length(), 0);

                int dataSocket = socket(AF_INET, SOCK_STREAM, 0);
                sockaddr_in dataAddr;
                dataAddr.sin_family = AF_INET;
                dataAddr.sin_addr.s_addr = INADDR_ANY;
                dataAddr.sin_port = htons(DATA_PORT);

                if (bind(dataSocket, (struct sockaddr*)&dataAddr, sizeof(dataAddr)) < 0) {
                    response = "Error: Could not bind data socket";
                } else if (listen(dataSocket, 1) < 0) {
                    response = "Error: Could not listen on data socket";
                } else {
                    int dataClientSocket = accept(dataSocket, NULL, NULL);
                    if (dataClientSocket < 0) {
                        response = "Error: Could not accept data connection";
                    } else {
                        char fileBuffer[BUFFER_SIZE];
                        while (!file.eof()) {
                            file.read(fileBuffer, BUFFER_SIZE);
                            send(dataClientSocket, fileBuffer, file.gcount(), 0);
                        }
                        close(dataClientSocket);
                        response = "File transfer complete";
                    }
                }
                close(dataSocket);
                file.close();
            } else {
                response = "Error: File not found";
            }
        } else {
            response = "Error: Unknown command";
        }

        send(clientSocket, response.c_str(), response.length(), 0);
        response.clear();
    }

    close(clientSocket);
}

int main() {
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(CONTROL_PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Could not bind to port" << std::endl;
        return 1;
    }

    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error: Could not listen on socket" << std::endl;
        return 1;
    }

    std::cout << "Server is listening on port " << CONTROL_PORT << std::endl;

    while (true) {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0) {
            std::cerr << "Error: Could not accept client connection" << std::endl;
            continue;
        }

        std::thread(handleClient, clientSocket).detach();
    }

    close(serverSocket);
    return 0;
}