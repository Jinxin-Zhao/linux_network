#include "thread_pool_server.h"

#include <arpa/inet.h>
#include <signal.h>
#include <chrono>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <stdexcept>

ThreadPool::ThreadPool(std::size_t thread_count) : stop_(false) {
    if (thread_count == 0) {
        thread_count = 1;
    }

    for (std::size_t i = 0; i < thread_count; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;

                {
                    std::unique_lock<std::mutex> lock(this->tasks_mutex_);
                    this->condition_.wait(lock, [this] {
                        return this->stop_.load() || !this->tasks_.empty();
                    });
                    if (this->stop_.load() && this->tasks_.empty()) {
                        return;
                    }
                    task = std::move(this->tasks_.front());
                    this->tasks_.pop();
                }

                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    stop_.store(true);
    condition_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

TcpConnection::TcpConnection(int sockfd, std::string peer_address)
    : socket_(sockfd), peer_address_(std::move(peer_address)) {
    setNonBlocking(sockfd);
}

void TcpConnection::operator()() {
    processRequest();
}

void TcpConnection::processRequest() {
    char buffer[kBufferSize];
    std::memset(buffer, 0, sizeof(buffer));

    while (true) {
        ssize_t bytes = recv(socket_.get(), buffer, sizeof(buffer) - 1, 0);
        if (bytes > 0) {
            std::string request(buffer, static_cast<std::size_t>(bytes));
            std::cout << "[worker] received from " << peer_address_ << ": " << request;

            std::string response = "Echo: " + request;
            ssize_t sent = send(socket_.get(), response.data(), response.size(), 0);
            if (sent < 0) {
                std::cerr << "[worker] failed to send response: " << std::strerror(errno) << '\n';
            }
            // continue to drain socket for this connection; when recv would block
            // we will exit the loop and let the connection close (simple model).
        } else if (bytes == 0) {
            // client closed the connection gracefully
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data available at the moment, wait until the next read.
                continue;
            }
            std::cerr << "[worker] recv failed: " << std::strerror(errno) << '\n';
            break;
        }
    }
}

ThreadPoolTcpServer::ThreadPoolTcpServer(std::string ip, uint16_t port, std::size_t thread_count)
    : ip_(std::move(ip)), port_(port), pool_(thread_count), running_(false) {}

ThreadPoolTcpServer::~ThreadPoolTcpServer() {
    stop();
}

void ThreadPoolTcpServer::createListenSocket() {
    int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        throw std::system_error(errno, std::generic_category(), "socket() failed");
    }

    listen_socket_ = SocketHandle(listen_fd);

    int option = 1;
    if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option)) < 0) {
        throw std::system_error(errno, std::generic_category(), "setsockopt(SO_REUSEADDR) failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(port_);
    if (inet_pton(AF_INET, ip_.c_str(), &address.sin_addr) != 1) {
        throw std::runtime_error("inet_pton() failed: invalid IP address");
    }

    if (bind(listen_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::system_error(errno, std::generic_category(), "bind() failed");
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        throw std::system_error(errno, std::generic_category(), "listen() failed");
    }

    setNonBlocking(listen_fd);
}

int ThreadPoolTcpServer::acceptConnection() {
    sockaddr_in client_address{};
    socklen_t client_addr_len = sizeof(client_address);
    int client_fd = accept(listen_socket_.get(), reinterpret_cast<sockaddr*>(&client_address), &client_addr_len);
    if (client_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return -1;
        }
        throw std::system_error(errno, std::generic_category(), "accept() failed");
    }
    return client_fd;
}

void ThreadPoolTcpServer::run() {
    createListenSocket();
    running_.store(true);
    std::cout << "[server] listening on " << ip_ << ":" << port_ << " with "
              << pool_.threadCount() << " worker threads\n";

    // install simple signal handler for graceful shutdown
    static std::atomic<bool> s_terminate{false};
    struct SigWrap { static void handler(int) { s_terminate.store(true); } };
    ::signal(SIGINT, SigWrap::handler);
    ::signal(SIGTERM, SigWrap::handler);

    while (running_.load() && !s_terminate.load()) {
        int client_fd = acceptConnection();
        if (client_fd == -1) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        sockaddr_in peer_addr{};
        socklen_t peer_addr_len = sizeof(peer_addr);
        if (getpeername(client_fd, reinterpret_cast<sockaddr*>(&peer_addr), &peer_addr_len) != 0) {
            std::cerr << "[server] getpeername failed: " << std::strerror(errno) << '\n';
        }
        char peer_buffer[INET_ADDRSTRLEN] = "unknown";
        inet_ntop(AF_INET, &peer_addr.sin_addr, peer_buffer, sizeof(peer_buffer));
        std::string peer_address = std::string(peer_buffer) + ":" + std::to_string(ntohs(peer_addr.sin_port));

        pool_.enqueue(TcpConnection(client_fd, std::move(peer_address)));
    }

    if (s_terminate.load()) {
        std::cout << "[server] shutdown requested by signal\n";
    }
}

void ThreadPoolTcpServer::stop() noexcept {
    if (running_.exchange(false)) {
        listen_socket_ = SocketHandle();
    }
}
