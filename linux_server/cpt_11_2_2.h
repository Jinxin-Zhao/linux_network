#ifndef _CPT_11_2_2_H_
#define _CPT_11_2_2_H_


// 利用上一节的Sort_timer_list
// 服务器程序通常要定期处理非活动连接:给容户端发一个重连请求，或者关闭该连接，或者其他
// Linux 在内核中提供了对连接是否处于活动状态的定期检查机制，我们可以通过socket选项 KEEPALIVE 来激活它。
// 不过使用这种方式将使得应用程序对连接的管理变得复杂。因此，我们可以考虑在应用层实现类似于KEEPALIVE的机制 ，
// 以管理所有长时间处于非活动状态的连接。比如，代码清单11-3 利用alarm 两数周期性地触发SIGALRM 信号，
// 该信号的信号处理两数利用管道通知主循环执行定时器链表上的定时任务一一关闭非活动的连接

#include "cpt_11_2_1.h"

namespace  n_timer_server {

    constexpr unsigned int FD_LIMIT = 65535;
    constexpr unsigned int MAX_EVENT_NUMBER = 1024;
    constexpr unsigned int TIMESLOT = 5;

    static int pipefd[2];
    static Sort_timer_list timer_list;
    static int epollfd = 0;

    int setnonblocking(int fd) {
        int old_option = fcntl(fd, F_GETFL);
        int new_option = old_option | O_NONBLOCK;
        fcntl(fd, F_SETFL, new_option);
        return old_option;
    }

    void addfd(int epollfd, int fd) {
        struct epoll_event event;
        event.data.fd = fd;
        event.events = EPOLLIN | EPOLLET;
        epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    }

    void sig_handler(int sig) {
        int save_errno = errno;
        int msg = sig;
        send(pipefd[1], (char*)&msg,1,0);
        errno = save_errno;
    }

    void add_sig(int sig) {
        struct sigaction sa;
        memset(&sa, '\0',sizeof(sa));
        sa.sa_handler = sig_handler;
        sa.sa_flags = SA_RESTART;
        sigfillset(&sa.sa_mask);
        assert(sigaction(sig, &sa, nullptr) != -1);
    }

    void timer_handler() {
        timer_list.tick();
        alarm(TIMESLOT);
    }

    // callback function: remove non-active connections & close them
    void cb_func(ClientData * user_data) {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, user_data->_sockfd, 0);
        assert(user_data);
        close(user_data->_sockfd);
        printf("close fd %d\n", user_data->_sockfd);
    }

    int test_timer_server(int argc, char *argv[]) {
        if (argc <= 2) {
            cout << "Usage: test_epoll <ip_address> <port_number> " << endl;
            return 1;
        }
        const char * ip = argv[1];
        int port = atoi(argv[2]);

        int ret = 0;
        struct sockaddr_in address;
        bzero(&address, sizeof(address));
        address.sin_family = AF_INET;
        inet_pton(AF_INET, ip, &address.sin_addr);
        address.sin_port = htons(port);

        int listenfd = socket(PF_INET,SOCK_STREAM,0);
        assert(listenfd >= 0);
        ret = bind(listenfd, (struct sockaddr *)&address,sizeof(address));
        assert(ret != -1);
        ret = listen(listenfd,5);
        assert(ret != -1);
        //
        epoll_event events[MAX_EVENT_NUMBER];
        int epollfd = epoll_create(5);
        assert(epollfd != -1);
        addfd(epollfd,listenfd);
        // socketpair函数通常用于本地进程间通信，而不适用于在网络上进行远程通信。对于网络通信，应使用其他套接字函数，如socket、bind、listen、connect等来创建和连接套接字。
        ret = socketpair(PF_UNIX,SOCK_STREAM,0,pipefd);
        assert(ret != -1);
        setnonblocking(pipefd[1]);
        addfd(epollfd,pipefd[0]);

        //
        add_sig(SIGALRM);
        add_sig(SIGTERM);
        bool stop_server = false;

        ClientData * users = new ClientData[FD_LIMIT];
        bool timeout = false;
        alarm(TIMESLOT);
        while (!stop_server) {
            int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
            if ((number < 0) && (errno != EINTR)) {
                cout << "epoll failuer\n" << endl;
                break;
            }
            for (int i = 0; i < number; i++) {
                int sockfd = events[i].data.fd;
                // solve new connection
                if (sockfd == listenfd) {
                    struct sockaddr_in client_address;
                    socklen_t cli_len = sizeof(client_address);
                    int connfd = accept(listenfd,(struct sockaddr*)&client_address,&cli_len);
                    addfd(epollfd, connfd);
                    users[connfd]._address = client_address;
                    users[connfd]._sockfd = connfd;
                    //
                    Util_timer * pTimer = new Util_timer;
                    pTimer->_user_data = &users[connfd];
                    pTimer->m_callback = cb_func;
                    time_t cur = time(nullptr);
                    pTimer->_expire = cur + 3 * TIMESLOT;
                    users[connfd]._timer = pTimer;
                    timer_list.add_timer(pTimer);
                } else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) {
                    //int sig;
                    char signals[1024];
                    ret = recv(pipefd[0],signals, sizeof(signals), 0);
                    if (ret == -1) {
                        // handle error
                        continue;
                    } else if (ret == 0) {
                        continue;
                    } else {
                        for (int i = 0; i < ret; ++i) {
                            switch (signals[i]) {
                                case SIGALRM: {
                                    timeout = true;
                                    break;
                                }
                                case SIGTERM: {
                                    stop_server = true;
                                }
                            }
                        }
                    }
                } else if (events[i].events & EPOLLIN) {
                    memset(users[sockfd]._buf, '\0', BUFFER_SIZE);
                    ret = recv(sockfd, users[sockfd]._buf, BUFFER_SIZE-1, 0);
                    printf("get %d bytes of client data %s from %d\n",ret,users[sockfd]._buf,sockfd);
                    Util_timer * timer = users[sockfd]._timer;
                    if (ret < 0) {
                        if (errno != EAGAIN) {
                            cb_func(&users[sockfd]);
                            if (timer) {
                                timer_list.del_timer(timer);
                            }
                        }
                    } else if (ret == 0) {
                        // the client close the connections, we close either
                        cb_func(&users[sockfd]);
                        if (timer) {
                            timer_list.del_timer(timer);
                        }
                    } else {
                        if (timer) {
                            time_t cur = time(NULL);
                            timer->_expire = cur + 3 * TIMESLOT;
                            timer_list.adjust_timer(timer);
                        }
                    }
                } else {
                    // others
                }
            }
            if (timeout) {
                timer_handler();
                timeout = false;
            }
        }
        close(listenfd);
        close(pipefd[1]);
        close(pipefd[0]);
        delete [] users;
        return 0;
    }
}





#endif