#define _WIN32_WINNT 0x0600
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>  // Ensure correct order after winsock2.h
#include <fstream>
#include <thread>      // For std::this_thread::sleep_for
#include <chrono>      // For time delays
#include <cstdio>      // For perror()
#include <string>      // For string handling

#pragma comment(lib, "ws2_32.lib")

// Configuration Constants
#define DEST_IP "192.168.1.17"  // Replace with your Kali Linux IP
#define SERVER_PORT 5050
#define INPUT_FILE "file.txt"      // File to send
#define CHUNK_SIZE 40              // Bytes to send at a time
#define SEND_DELAY_MS 1000         // 1 second delay between chunks

// Toggle flags for TCP optimizations
bool enableNagle = true;      // TCP_NODELAY = false means Nagle is enabled
bool useDelay = false;        // Whether to use artificial delay between sends

// Function to initialize Winsock
bool initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "Error: WSAStartup failed with code " << result << std::endl;
        return false;
    }
    return true;
}

// Function to create and configure a TCP socket
SOCKET createTcpSocket() {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "Error: Failed to create socket. Code: " << WSAGetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    // Configure Nagle's Algorithm based on flag
    // When enableNagle is false, we set TCP_NODELAY to true (1)
    int disableFlag = enableNagle ? 0 : 1;
    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&disableFlag, sizeof(disableFlag)) == SOCKET_ERROR) {
        std::cerr << "Error: Failed to " << (enableNagle ? "enable" : "disable") 
                  << " Nagle's algorithm. Code: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        return INVALID_SOCKET;
    }

    std::cout << "Nagle's Algorithm: " << (enableNagle ? "Enabled" : "Disabled") << std::endl;
    std::cout << "Artificial Delay: " << (useDelay ? "Enabled (" + std::to_string(SEND_DELAY_MS) + "ms)" : "Disabled") << std::endl;

    return clientSocket;
}

// Function to connect to the server
bool connectToServer(SOCKET& clientSocket) {
    sockaddr_in serverAddress = {};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);

    // Convert IP address
    serverAddress.sin_addr.s_addr = inet_addr(DEST_IP);
    if (serverAddress.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Error: Invalid IP address." << std::endl;
        return false;
    }

    // Establish connection
    if (connect(clientSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        std::cerr << "Error: Connection to server failed. Code: " << WSAGetLastError() << std::endl;
        return false;
    }

    std::cout << "Connected to server successfully!" << std::endl;
    return true;
}

// Function to send a file over a TCP connection
void sendFile(SOCKET& clientSocket) {
    std::ifstream inputFile(INPUT_FILE, std::ios::binary);
    if (!inputFile) {
        std::cerr << "Error: Cannot open file: " << INPUT_FILE << std::endl;
        return;
    }

    char dataBuffer[CHUNK_SIZE];
    int totalBytesSent = 0;
    auto startTime = std::chrono::high_resolution_clock::now();

    while (inputFile.read(dataBuffer, CHUNK_SIZE) || inputFile.gcount() > 0) {
        int bytesToSend = inputFile.gcount();

        // Apply artificial delay before sending if enabled
        if (useDelay) {
            std::cout << "Applying delay of " << SEND_DELAY_MS << "ms before sending..." << std::endl;
            Sleep(SEND_DELAY_MS);
        }

        int sentBytes = send(clientSocket, dataBuffer, bytesToSend, 0);
        if (sentBytes == SOCKET_ERROR) {
            std::cerr << "Error: Data send failed. Code: " << WSAGetLastError() << std::endl;
            inputFile.close();
            return;
        }

        totalBytesSent += sentBytes;
        std::cout << "Sent " << sentBytes << " bytes. Total: " << totalBytesSent << " bytes." << std::endl;
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "File sent successfully. Total bytes sent: " << totalBytesSent << " bytes." << std::endl;
    std::cout << "Total transmission time: " << duration << "ms" << std::endl;
    std::cout << "Average throughput: " << (totalBytesSent * 1000.0 / duration) << " bytes/second" << std::endl;

    inputFile.close();
}

// Cleanup and close the socket
void cleanup(SOCKET& clientSocket) {
    shutdown(clientSocket, SD_SEND);
    closesocket(clientSocket);
    WSACleanup();
}

void displayUsage() {
    std::cout << "TCP File Transfer With Nagle and Delay Options" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  1. Nagle ON, Delay OFF(" << SEND_DELAY_MS << "ms)" << std::endl;;
    std::cout << "  2. Nagle OFF, Delay OFF(" << SEND_DELAY_MS << "ms)" << std::endl;;
    std::cout << "  3. Nagle ON, Delay ON (" << SEND_DELAY_MS << "ms)" << std::endl;
    std::cout << "  4. Nagle OFF, Delay ON (" << SEND_DELAY_MS << "ms)" << std::endl;
    std::cout << "Enter option (1-4): ";
}

int main() {
    // Display options and get user choice
    displayUsage();
    int option;
    std::cin >> option;

    // Configure settings based on user choice
    switch (option) {
        case 1:
            enableNagle = true;
            useDelay = false;
            break;
        case 2:
            enableNagle = false;
            useDelay = false;
            break;
        case 3:
            enableNagle = true;
            useDelay = true;
            break;
        case 4:
            enableNagle = false;
            useDelay = true;
            break;
        default:
            std::cout << "Invalid option. Using default: Nagle ON, Delay OFF" << std::endl;
            enableNagle = true;
            useDelay = false;
    }

    // Step 1: Initialize Winsock
    if (!initializeWinsock()) {
        return 1;
    }

    // Step 2: Create TCP socket
    SOCKET clientSocket = createTcpSocket();
    if (clientSocket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    // Step 3: Connect to the server
    if (!connectToServer(clientSocket)) {
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Step 4: Send file
    sendFile(clientSocket);

    // Step 5: Cleanup
    cleanup(clientSocket);

    std::cout << "Press Enter to exit..." << std::endl;
    std::cin.ignore();
    std::cin.get();

    return 0;
}
