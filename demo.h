#ifndef __DEMO_H_
#define __DEMO_H_

#include "common.h"


void pure_ET() {
    // 当用户输入一组字符，这组字符被送入buffer，字符停留在buffer中，buffer由空-》非空，epoll_wait返回读就绪
    // 之后程序再次执行epoll_wait，此时虽然buffer中有内容可读，但是epoll_wait并不返回（ET），导致epoll_wait阻塞。
    //（底层原因是ET下就绪fd的epitem只被放入rdlist一次）
    // 用户再次输入一组字符，导致buffer中的内容增多，根据我们上节的分析这将导致fd状态的改变，是对应的epitem再次加入rdlist，从而使epoll_wait返回读就绪，再次输出“Welcome to epoll”。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDIN_FILENO;
    // default: LT
    //监听读状态同时设置ET模式
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                printf("Welcome to epoll\n");
            }
        }
    }
}

void pure_LT() {
    //程序陷入死循环，因为用户输入任意数据后，数据被送入buffer且没有被读出，
    // 所以LT模式下每次epoll_wait都认为buffer可读返回读就绪。导致每次都会输出”welcome to epoll's world！”。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDIN_FILENO;
    // default: LT
    ev.events = EPOLLIN;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                printf("Welcome to epoll\n");
            }
        }
    }
}

void readout_LT() {
    //LT模式，每次epoll_wait()返回读就绪的时候我们都将buffer的内容读出来，导致buffer再次清空，
    //下次调用epoll_wait就会阻塞，当用户有输入时，缓冲区又有数据继续使epoll_wait处于就绪状态返回。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDIN_FILENO;
    // default: LT
    ev.events = EPOLLIN;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                char buf[1024] = {0};
                read(STDIN_FILENO,buf,sizeof(buf));
                printf("Welcome to epoll, buf %s\n", buf);
            }
        }
    }
}

void Reset_ET() {
    // 程序依然使用ET，但是每次读就绪后都主动的再次MOD IN事件，我们发现程序再次出现死循环，也就是每次返回读就绪。
    // 但是注意，如果我们将MOD改为ADD，将不会产生任何影响。别忘了每次ADD一个描述符都会在epitem组成的红黑树中添加一个项，
    // 我们之前已经ADD过一次，再次ADD将阻止添加，所以在次调用ADD IN事件不会有任何影响。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDIN_FILENO;
    // default: LT
    ev.events = EPOLLIN | EPOLLET;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                printf("Welcome to epoll\n");
                ev.data.fd = STDIN_FILENO;
                ev.events = EPOLLIN | EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,STDIN_FILENO,&ev);
            }
        }
    }
}

////////////////STDOUT_FILENO
void Write_ET_loop() {
    // 首先初始buffer为空，buffer中有空间可写，这时无论是ET还是LT都会将对应的epitem加入rdlist，导致epoll_wait返回写就绪；
    // 因为标准输出为控制台的时候缓冲是行缓冲，所以换行符导致buffer中的内容清空。当有旧数据被发送走时，即buffer中待写的内容变少得时候会触发fd状态的改变。
    // 所以下次epoll_wait会返回写就绪。如此循环往复。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDOUT_FILENO;
    // default: LT
    ev.events = EPOLLOUT | EPOLLET;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDOUT_FILENO) {
                printf("Welcome to epoll\n");
            }
        }
    }
}


void Write_ET_hang() {
    // 与上个函数相比，printf移除了换行符，程序成挂起状态。由于没有换行符，缓冲区内容一直存在、
    // 下次epoll_wait的时候，虽然有写空间但是ET模式下不再返回写就绪（ET模式返回一次就绪后移除epitem）。
    // 之后虽然buffer仍然可写，但是由于对应epitem已经不再rdlist中，就不会对其就绪fd的events检测了。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDOUT_FILENO;
    // default: LT
    ev.events = EPOLLOUT | EPOLLET;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDOUT_FILENO) {
                printf("Welcome to epoll");
            }
        }
    }
}

void Write_LT_loop() {
    // 由于没有换行符，buffer中依旧有内容，虽然buffer没有输出清空，但是LT模式下只要buffer有写空间就返回写就绪
    // 所以会一直输出"Welcome to epoll"，当buffer满的时候buffer会自动刷清输出，同样会造成epoll_wait再次写就绪，如此往复。
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDOUT_FILENO;
    // default: LT
    ev.events = EPOLLOUT;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDOUT_FILENO) {
                printf("Welcome to epoll");
            }
        }
    }
}

void Write_ET_RELoad() {
    int epfd, nfds;
    // ev用于注册事件, event[]用于返回要处理的事件
    struct epoll_event ev, events[5];
    epfd = epoll_create(1);
    ev.data.fd = STDOUT_FILENO;
    // default: LT
    ev.events = EPOLLOUT | EPOLLET;
    //监听读状态同时设置ET模式
    //ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDOUT_FILENO, &ev);
    for(;;) {
        nfds = epoll_wait(epfd, events, 5, -1);
        for (auto i = 0; i < nfds; i++) {
            if (events[i].data.fd == STDOUT_FILENO) {
                printf("Welcome to epoll");
                ev.data.fd = STDOUT_FILENO;
                ev.events = EPOLLOUT | EPOLLET;
                epoll_ctl(epfd,EPOLL_CTL_MOD,STDOUT_FILENO,&ev);
            }
        }
    }
}

//经过前面的案例分析，我们已经了解到，当epoll工作在ET模式下时，对于读操作，如果read一次没有读尽buffer中的数据，那么下次将得不到读就绪的通知，造成buffer中已有的数据无机会读出，除非有新的数据再次到达。对于写操作，主要是因为ET模式下fd通常为非阻塞造成的一个问题——如何保证将用户要求写的数据写完。
//要解决上述两个ET模式下的读写问题，我们必须实现：
//1. 对于读，只要buffer中还有数据就一直读；
//2. 对于写，只要buffer还有空间且用户请求写的数据还未写完，就一直写。

//ET模式下的accept问题
//请思考以下一种场景：在某一时刻，有多个连接同时到达，服务器的 TCP 就绪队列瞬间积累多个就绪连接，由于是边缘触发模式，epoll 只会通知一次，accept 只处理一个连接，导致 TCP 就绪队列中剩下的连接都得不到处理。在这种情形下，我们应该如何有效的处理呢？

//解决的方法是：解决办法是用 while 循环抱住 accept 调用，处理完 TCP 就绪队列中的所有连接后再退出循环。如何知道是否处理完就绪队列中的所有连接呢？ accept  返回 -1 并且 errno 设置为 EAGAIN 就表示所有连接都处理完。

//关于ET的accept问题，这篇博文的参考价值很高，如果有兴趣，可以链接过去围观一下。





#endif