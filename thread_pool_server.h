#ifndef THREAD_POOL_SERVER_H
#define THREAD_POOL_SERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

// A small RAII wrapper for a socket file descriptor.
class SocketHandle {
public:
    explicit SocketHandle(int fd = -1) noexcept : fd_(fd) {}
    ~SocketHandle() {
        if (fd_ != -1) {
            ::close(fd_);
        }
    }
    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;
    SocketHandle(SocketHandle&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }
    SocketHandle& operator=(SocketHandle&& other) noexcept {
        if (this != &other) {
            if (fd_ != -1) {
                ::close(fd_);
            }
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    int get() const noexcept { return fd_; }
    int release() noexcept { int tmp = fd_; fd_ = -1; return tmp; }
    explicit operator bool() const noexcept { return fd_ != -1; }

private:
    int fd_;
};

// Modern C++ thread pool implementation.
// The pool launches several worker threads that sleep on a condition variable.
// When the main thread enqueues a new task, one worker wakes up and executes it.
class ThreadPool {
public:
    explicit ThreadPool(std::size_t thread_count);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> result = task->get_future();
        {
            std::lock_guard<std::mutex> lock(tasks_mutex_);
            if (stop_.load()) {
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        condition_.notify_one();
        return result;
    }

    std::size_t threadCount() const noexcept { return workers_.size(); }

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex tasks_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};

inline void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        throw std::system_error(errno, std::generic_category(), "fcntl(F_GETFL) failed");
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        throw std::system_error(errno, std::generic_category(), "fcntl(F_SETFL) failed");
    }
}

// A lightweight connection handler owned by a worker thread.
class TcpConnection {
public:
    explicit TcpConnection(int sockfd, std::string peer_address);
    void operator()();

private:
    void processRequest();

private:
    SocketHandle socket_;
    std::string peer_address_;
    static constexpr std::size_t kBufferSize = 1024;
};

// A simple TCP server that accepts client sockets on the main thread
// and uses a thread pool to handle each connection.
// The main thread is responsible only for accept(), while workers process I/O.
class ThreadPoolTcpServer {
public:
    ThreadPoolTcpServer(std::string ip, uint16_t port, std::size_t thread_count);
    ~ThreadPoolTcpServer();

    ThreadPoolTcpServer(const ThreadPoolTcpServer&) = delete;
    ThreadPoolTcpServer& operator=(const ThreadPoolTcpServer&) = delete;

    void run();
    void stop() noexcept;

private:
    void createListenSocket();
    int acceptConnection();

private:
    std::string ip_;
    uint16_t port_;
    ThreadPool pool_;
    SocketHandle listen_socket_;
    std::atomic<bool> running_;
};

#endif // THREAD_POOL_SERVER_H
