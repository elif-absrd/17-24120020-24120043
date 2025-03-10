// Server-side code (Kali Linux)

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <cstring>
#include <chrono>
#include <errno.h>
#include <arpa/inet.h>
#include <fstream>

#define PORT 4999
#define BUFFER_SIZE 40
#define OUTPUT_FILE "received_file.txt"

using namespace std;

// Function to configure socket TCP options
void configureSocket(int sockfd, bool nagle, bool delayedAck) {
    // Configure Nagle's algorithm (TCP_NODELAY = 1 turns OFF Nagle)
    int flag = nagle ? 0 : 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0) {
        perror("Failed to configure Nagle's algorithm");
    }

    // Configure Delayed ACK (TCP_QUICKACK = 1 turns OFF delayed ACK)
    flag = delayedAck ? 0 : 1;
    if (setsockopt(sockfd, IPPROTO_TCP, TCP_QUICKACK, &flag, sizeof(flag)) < 0) {
        perror("Failed to configure Delayed ACK");
    }

    cout << "Socket configured: Nagle " << (nagle ? "ON" : "OFF") 
         << " | Delayed-ACK " << (delayedAck ? "ON" : "OFF") << endl;
}

// Function to receive data and measure performance
void receiveFileAndMeasure(int sockfd, bool nagle, bool delayedAck) {
    char buffer[BUFFER_SIZE];
    int bytesReceived = 0;
    int totalBytes = 0;
    int packetsReceived = 0;
    int maxPacketSize = 0;
    ofstream outFile(OUTPUT_FILE, ios::binary);

    cout << "\n--- Starting transfer with configuration ---" << endl;
    cout << "Nagle: " << (nagle ? "ON" : "OFF") << " | Delayed-ACK: " << (delayedAck ? "ON" : "OFF") << endl;
    
    // Configure the socket with specified options
    configureSocket(sockfd, nagle, delayedAck);
    
    // Start timing
    auto start = chrono::high_resolution_clock::now();

    // Receive data
    while ((bytesReceived = recv(sockfd, buffer, BUFFER_SIZE, 0)) > 0) {
        totalBytes += bytesReceived;
        packetsReceived++;
        
        if (bytesReceived > maxPacketSize) {
            maxPacketSize = bytesReceived;
        }
        
        // Write received data to file
        if (outFile.is_open()) {
            outFile.write(buffer, bytesReceived);
        }
        
        // Print progress every 10 packets
        if (packetsReceived % 10 == 0) {
            cout << "Received " << totalBytes << " bytes so far..." << endl;
        }
    }

    // End timing
    auto end = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    // Close the output file
    if (outFile.is_open()) {
        outFile.close();
    }

    // Report statistics
    cout << "\n--- Transfer Complete ---" << endl;
    cout << "Configuration: Nagle " << (nagle ? "ON" : "OFF") 
         << " | Delayed-ACK " << (delayedAck ? "ON" : "OFF") << endl;
    cout << "Total Bytes Received: " << totalBytes << " bytes" << endl;
    cout << "Total Time: " << duration.count() << " ms" << endl;
    
    // Avoid division by zero
    double throughput = 0;
    if (duration.count() > 0) {
        throughput = (totalBytes * 1000.0) / duration.count();
    }
    
    cout << "Throughput: " << throughput << " bytes/sec" << endl;
    cout << "Max Packet Size: " << maxPacketSize << " bytes" << endl;
    cout << "Packets Received: " << packetsReceived << endl;
    cout << "Average Packet Size: " << (packetsReceived > 0 ? totalBytes/packetsReceived : 0) << " bytes" << endl;
    cout << "-------------------------------" << endl;
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    
    // Create the server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt SO_REUSEADDR failed");
        exit(EXIT_FAILURE);
    }
    
    // Prepare the sockaddr_in structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    cout << "Server started. Listening on port " << PORT << "..." << endl;
    
    // Accept incoming connections and handle them one by one
    while (true) {
        cout << "Waiting for client connection..." << endl;
        
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("Accept failed");
            continue;  // Try again
        }
        
        // Print client information
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(address.sin_addr), client_ip, INET_ADDRSTRLEN);
        cout << "Client connected from " << client_ip << ":" << ntohs(address.sin_port) << endl;
        
        // Get user input for configuration
        bool nagle, delayedAck;
        int option;
        
        cout << "Select configuration:" << endl;
        cout << "1. Nagle ON, DelayedACK ON" << endl;
        cout << "2. Nagle ON, DelayedACK OFF" << endl;
        cout << "3. Nagle OFF, DelayedACK ON" << endl;
        cout << "4. Nagle OFF, DelayedACK OFF" << endl;
        cout << "Enter option (1-4): ";
        cin >> option;
        
        switch (option) {
            case 1:
                nagle = true;
                delayedAck = true;
                break;
            case 2:
                nagle = true;
                delayedAck = false;
                break;
            case 3:
                nagle = false;
                delayedAck = true;
                break;
            case 4:
                nagle = false;
                delayedAck = false;
                break;
            default:
                cout << "Invalid option. Using default: Nagle ON, DelayedACK ON" << endl;
                nagle = true;
                delayedAck = true;
        }
        
        // Receive file and measure performance
        receiveFileAndMeasure(client_socket, nagle, delayedAck);
        
        // Close the connection
        close(client_socket);
        cout << "Connection closed. Ready for next client." << endl;
    }
    
    // Close the server socket (this line is actually never reached in this example)
    close(server_fd);
    
    return 0;
}
