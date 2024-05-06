#ifndef __SOCK_TIMER_H_
#define __SOCK_TIMER_H_


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

int timeout_connect(const char* ip, int port, int time){
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd>0);
    // 通过选项设置超时时间类型为timeval
    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof(timeout);
    ret = setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
    assert(ret!=-1);
    ret = connect(sockfd, (struct sockaddr*)&address, sizeof(address));

    if(ret == -1){
        // 超时错误号对应的EINPROGRESS
        if(errno == EINPROGRESS){
            printf("connecting timeout, process timeout\n");
            return -1;
        }
        printf("error occur when connectiong to server\n");
        return -1;
    }
    return sockfd;
}

int test_sock_timer(int argc, char* argv[]){
    if(argc<=2){
        printf("usage: %s ip_adress port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd = timeout_connect(ip, port, 10);
    if(sockfd < 0){
        return 1;
    }
    return 0;
}



#endif