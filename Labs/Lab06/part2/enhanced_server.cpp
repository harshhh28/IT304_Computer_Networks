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
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <chrono>

const int CONTROL_PORT = 8080;
const int DATA_PORT = 8089;
const int BUFFER_SIZE = 4096;
const int MAX_THREADS = 32;

struct Task {
    int clientSocket;
    std::string command;
};

std::queue<Task> taskQueue;
std::mutex queueMutex;
std::condition_variable condition;
std::atomic<bool> stop{false};
std::vector<std::thread> workerThreads;

void handleClient(int clientSocket, const std::string& command) {
    char buffer[BUFFER_SIZE];
    std::string response;

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
}

void workerThread() {
    while (!stop) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            condition.wait(lock, [] { return !taskQueue.empty() || stop; });
            if (stop && taskQueue.empty()) return;
            task = taskQueue.front();
            taskQueue.pop();
        }
        handleClient(task.clientSocket, task.command);
        close(task.clientSocket);
    }
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

    if (listen(serverSocket, SOMAXCONN) < 0) {
        std::cerr << "Error: Could not listen on socket" << std::endl;
        return 1;
    }

    std::cout << "Server is listening on port " << CONTROL_PORT << std::endl;

    for (int i = 0; i < MAX_THREADS; ++i) {
        workerThreads.emplace_back(workerThread);
    }

    while (!stop) {
        int clientSocket = accept(serverSocket, NULL, NULL);
        if (clientSocket < 0) {
            std::cerr << "Error: Could not accept client connection" << std::endl;
            continue;
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE, 0);
        if (bytesReceived > 0) {
            std::string command(buffer);
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                taskQueue.push({clientSocket, command});
            }
            condition.notify_one();
        } else {
            close(clientSocket);
        }
    }

    stop = true;
    condition.notify_all();
    for (auto& thread : workerThreads) {
        thread.join();
    }

    close(serverSocket);
    return 0;
}