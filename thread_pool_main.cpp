#include "thread_pool_server.h"
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
        return 1;
    }

    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));

    try {
        // The server accepts sockets on the main thread and assigns them to worker threads.
        ThreadPoolTcpServer server(ip, port, std::thread::hardware_concurrency());
        std::cout << "Starting thread-pool TCP server...\n";
        server.run();
    } catch (const std::exception& ex) {
        std::cerr << "Server failure: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
