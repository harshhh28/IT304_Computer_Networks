#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>

const int CONTROL_PORT = 8080;
const int DATA_PORT = 8089;
const int BUFFER_SIZE = 1024;

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <server_ip>" << std::endl;
        return 1;
    }

    int controlSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CONTROL_PORT);
    inet_pton(AF_INET, argv[1], &serverAddr.sin_addr);

    if (connect(controlSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Could not connect to server" << std::endl;
        return 1;
    }

    std::cout << "Connected to server. Enter commands (DIR, GET filename, or EXIT):" << std::endl;

    std::string command;
    char buffer[BUFFER_SIZE];

    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);

        if (command == "EXIT") break;

        send(controlSocket, command.c_str(), command.length(), 0);

        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(controlSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        std::string response(buffer);
        std::cout << response << std::endl;

        if (command.substr(0, 3) == "GET" && response == "Ready to transfer") {
            std::string filename = command.substr(4);
            std::ofstream file(filename, std::ios::binary);

            int dataSocket = socket(AF_INET, SOCK_STREAM, 0);
            sockaddr_in dataAddr = serverAddr;
            dataAddr.sin_port = htons(DATA_PORT);

            if (connect(dataSocket, (struct sockaddr*)&dataAddr, sizeof(dataAddr)) < 0) {
                std::cerr << "Error: Could not connect to data port" << std::endl;
                continue;
            }

            auto start = std::chrono::high_resolution_clock::now();

            while (true) {
                memset(buffer, 0, BUFFER_SIZE);
                int bytesReceived = recv(dataSocket, buffer, BUFFER_SIZE, 0);
                if (bytesReceived <= 0) break;
                file.write(buffer, bytesReceived);
            }

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

            std::cout << "File transfer completed in " << duration.count() << " ms" << std::endl;

            close(dataSocket);
            file.close();
        }
    }

    close(controlSocket);
    return 0;
}