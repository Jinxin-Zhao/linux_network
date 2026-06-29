#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <message>\n";
        return 1;
    }

    std::string server_ip = argv[1];
    uint16_t server_port = static_cast<uint16_t>(std::stoi(argv[2]));
    std::string message = argv[3];

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "socket() failed: " << std::strerror(errno) << '\n';
        return 1;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr) != 1) {
        std::cerr << "inet_pton() failed: invalid IP address\n";
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) < 0) {
        std::cerr << "connect() failed: " << std::strerror(errno) << '\n';
        close(sockfd);
        return 1;
    }

    std::string payload = message + "\r\n";
    if (send(sockfd, payload.data(), payload.size(), 0) < 0) {
        std::cerr << "send() failed: " << std::strerror(errno) << '\n';
        close(sockfd);
        return 1;
    }

    // Read the server response; this client sends one request and waits for one reply.
    char buffer[1024];
    ssize_t received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (received < 0) {
        std::cerr << "recv() failed: " << std::strerror(errno) << '\n';
    } else if (received == 0) {
        std::cout << "Server closed connection before sending a response.\n";
    } else {
        buffer[received] = '\0';
        std::cout << "Received: " << buffer;
    }

    close(sockfd);
    return 0;
}
