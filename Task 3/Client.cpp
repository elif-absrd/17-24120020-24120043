// Client-side code (Windows)

#define _WIN32_WINNT 0x0600
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>
#include <thread>
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "192.168.200.133"
#define PORT 4999
#define FILE_PATH "file.txt"
#define BUFFER_SIZE 40
#define FILE_SIZE 4096
#define DELAY_MS 1000

using namespace std;

void configureSocket(SOCKET sock, bool nagle) {
    int flag = nagle ? 0 : 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(flag));
}

void sendFile(SOCKET sock) {
    ifstream file(FILE_PATH, ios::binary);
    if (!file) {
        cerr << "Error: File not found." << endl;
        return;
    }

    char buffer[BUFFER_SIZE];
    int totalBytesSent = 0;

    while (totalBytesSent < FILE_SIZE && file.read(buffer, BUFFER_SIZE)) {
        send(sock, buffer, BUFFER_SIZE, 0);
        totalBytesSent += BUFFER_SIZE;
        std::this_thread::sleep_for(chrono::milliseconds(DELAY_MS));
    }

    cout << "File sent successfully. Total bytes sent: " << totalBytesSent << " bytes\n";

    file.close();
    shutdown(sock, SD_SEND);
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr));
    cout << "Connected to server successfully!\n";

    bool nagleOptions[] = {true, true, false, false};

    for (int i = 0; i < 4; ++i) {
        configureSocket(sock, nagleOptions[i]);
        sendFile(sock);
    }

    closesocket(sock);
    WSACleanup();

    return 0;
}
