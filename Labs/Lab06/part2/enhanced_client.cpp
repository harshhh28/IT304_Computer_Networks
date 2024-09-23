#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <vector>

const int CONTROL_PORT = 8080;
const int DATA_PORT = 8089;
const int BUFFER_SIZE = 4096;

double transferFile(const std::string& serverIP, const std::string& filename) {
    int controlSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(CONTROL_PORT);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    if (connect(controlSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error: Could not connect to server" << std::endl;
        return -1;
    }

    std::string command = "GET " + filename;
    send(controlSocket, command.c_str(), command.length(), 0);

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytesReceived = recv(controlSocket, buffer, BUFFER_SIZE, 0);
    if (bytesReceived <= 0 || std::string(buffer) != "Ready to transfer") {
        std::cerr << "Error: " << buffer << std::endl;
        close(controlSocket);
        return -1;
    }

    int dataSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in dataAddr = serverAddr;
    dataAddr.sin_port = htons(DATA_PORT);

    if (connect(dataSocket, (struct sockaddr*)&dataAddr, sizeof(dataAddr)) < 0) {
        std::cerr << "Error: Could not connect to data port" << std::endl;
        close(controlSocket);
        return -1;
    }

    std::ofstream file(filename, std::ios::binary);
    auto start = std::chrono::high_resolution_clock::now();

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(dataSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;
        file.write(buffer, bytesReceived);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    close(dataSocket);
    close(controlSocket);
    file.close();

    return duration.count() / 1000.0; // Convert to seconds
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <filename> <num_transfers>" << std::endl;
        return 1;
    }

    std::string serverIP = argv[1];
    std::string filename = argv[2];
    int numTransfers = std::stoi(argv[3]);

    std::vector<std::thread> threads;
    std::vector<double> transferTimes(numTransfers);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numTransfers; ++i) {
        threads.emplace_back([&, i] {
            transferTimes[i] = transferFile(serverIP, filename);
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto totalDuration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    double avgTime = 0;
    int successfulTransfers = 0;
    for (double time : transferTimes) {
        if (time >= 0) {
            avgTime += time;
            successfulTransfers++;
        }
    }
    avgTime /= successfulTransfers;

    std::cout << "Number of clients: " << numTransfers << std::endl;
    std::cout << "Successful transfers: " << successfulTransfers << std::endl;
    std::cout << "Average transfer time: " << avgTime << " seconds" << std::endl;
    std::cout << "Total time for all transfers: " << totalDuration.count() / 1000.0 << " seconds" << std::endl;

    return 0;
}