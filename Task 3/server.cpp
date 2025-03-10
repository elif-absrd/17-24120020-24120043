// Server-side code (Kali Linux)

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <errno.h>

#define PORT 4999
#define BUFFER_SIZE 40
#define FILE_SIZE 4096

using namespace std;

void configureSocket(int sockfd, bool nagle, bool delayedAck) {
    int flag = nagle ? 0 : 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    flag = delayedAck ? 0 : 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag));
}

void measurePerformance(int sockfd, bool nagle, bool delayedAck) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0, totalBytes = 0, maxPacketSize = 0, packetsReceived = 0;

    auto start = chrono::high_resolution_clock::now();

    while (totalBytes < FILE_SIZE) {
        bytesReceived = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (bytesReceived <= 0) break;

        totalBytes += bytesReceived;
        packetsReceived++;
        if (bytesReceived > maxPacketSize) maxPacketSize = bytesReceived;
    }

    auto end = chrono::high_resolution_clock::now();
    double elapsedTime = chrono::duration_cast<chrono::seconds>(end - start).count();

    cout << "\nConfiguration: Nagle " << (nagle ? "ON" : "OFF") << " | Delayed-ACK " << (delayedAck ? "ON" : "OFF") << "\n";
    cout << "Total Bytes Received: " << totalBytes << " bytes\n";
    cout << "Throughput: " << (totalBytes / elapsedTime) << " bytes/sec\n";
    cout << "Max Packet Size: " << maxPacketSize << " bytes\n";
    cout << "Packets Received: " << packetsReceived << "\n";
    cout << "Goodput: " << (totalBytes / elapsedTime) << " bytes/sec\n";
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    cout << "Listening on port " << PORT << "...\n";

    new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
    if (new_socket < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    cout << "Client connected!\n";

    bool nagleOptions[] = {true, true, false, false};
    bool delayedAckOptions[] = {true, false, true, false};

    for (int i = 0; i < 4; ++i) {
        configureSocket(new_socket, nagleOptions[i], delayedAckOptions[i]);
        measurePerformance(new_socket, nagleOptions[i], delayedAckOptions[i]);
    }

    close(new_socket);
    close(server_fd);
    return 0;
}
