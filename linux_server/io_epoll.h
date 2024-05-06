#ifndef  IO_EPOLL_H
#define  IO_EPOLL_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

constexpr unsigned int MAX_EVENT_NUMBER = 1024;
constexpr unsigned int BUFFER_SIZE = 10;

namespace IO_EPOLL {
// set fd to non-blocking
    int setnonblocking(int fd) {
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_option;
    }

    void addfd(int epollfd, int fd, bool enable_et = false) {
        epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN;
        if (enable_et) {
            event.events |= EPOLLET;
        }
        epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
        setnonblocking(fd);
    }

    void lt(epoll_event *events, int number, int epollfd, int listenfd) {
        char buf[BUFFER_SIZE];
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
                addfd(epollfd, connfd, false);
            } else if (events[i].events & EPOLLIN) {
                printf("[%s] event trigger once\n", __FUNCTION__);
                memset(buf, '\0', BUFFER_SIZE);
                int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                if (ret <= 0) {
                    close(sockfd);
                    continue;
                }
                printf("[%s] get %d bytes of content: %s\n", __FUNCTION__, ret, buf);
            }
        }
    }

//每个使用ET模式的文件描述符都应该是非阻塞的，如果文件描述符是阻塞的，那么读或写操作将会因为没有后续的事件而一直处于阻塞状态（饥渴状态）
    void et(epoll_event *events, int number, int epollfd, int listenfd) {
        char buf[BUFFER_SIZE];
        for (int i = 0; i < number; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *) &client_address, &client_addrlength);
                addfd(epollfd, connfd, true);
            } else if (events[i].events & EPOLLIN) {
                // ET模式不会被重复触发，我们要循环读取数据，以确保把socket读缓存中的所有数据读出
                printf("event trigger once\n");
                while (1) {
                    memset(buf, '\0', BUFFER_SIZE);
                    int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
                    if (ret < 0) {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                            printf("read later\n");
                            break;
                        }
                        close(sockfd);
                        break;
                    } else if (ret == 0) {
                        close(sockfd);
                    } else {
                        printf("get %d bytes of content : %s\n", ret, buf);
                    }
                }
            } else {
                printf("something else happened\n");
            }
        }
    }

    int test_epoll_func(char *ipaddress, int port) {
        const char *ip = ipaddress;
        int ret = 0;
        struct sockaddr_in address;
        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &address.sin_addr);
        address.sin_port = htons(port);
        int listenfd = socket(PF_INET, SOCK_STREAM, 0);
        assert(listenfd >= 0);
        ret = bind(listenfd, (struct sockaddr *) &address, sizeof(address));
        assert(ret != -1);
        ret = listen(listenfd, 5);
        assert(ret != -1);

        epoll_event events[MAX_EVENT_NUMBER];
        int epollfd = epoll_create(5);
        assert(epollfd != -1);
        addfd(epollfd, listenfd, true);
        while (1) {
            int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
            if (ret < 0) {
                printf("epoll failure\n");
                break;
            }
            lt(events, ret, epollfd, listenfd);
        }
        close(listenfd);
        return 0;
    }

}


#endif