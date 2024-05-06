#ifndef __IO_SHARED_MEMORY_H_
#define __IO_SHARED_MEMORY_H_

#include "../common.h"

int test_shm_write() {
    key_t key = ftok("./myshm", 0);
    int shmid = shmget(key, 0x400000, IPC_CREAT | 0666);
    char * p = (char*)shmat(shmid,nullptr,0);
    memset(p, 'A', 0x400000);
    shmdt(p);
    return 0;
}

int test_shm_read() {
    key_t key = ftok("./myshm", 0);
    int shmid = shmget(key, 0x400000, IPC_CREAT | 0666);
    char * p = (char*)shmat(shmid,nullptr,0);
    printf("%c %c %c %c\n",p[0],p[1],p[3],p[4]);
    shmdt(p);
    return 0;
}

// POSIX shared memory
int test_shm_posix_write() {
    int fd = shm_open("posixsm",O_CREAT | O_RDWR , 0666);
    ftruncate(fd,0x400000);
    char * p = (char*)mmap(nullptr, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    memset(p, 'A', 0x400000);
    munmap(p, 0x400000);
    return 0;
}

int test_shm_posix_read() {
    int fd = shm_open("posixsm",O_CREAT | O_RDWR , 0666);
    ftruncate(fd,0x400000);
    char * p = (char*)mmap(nullptr, 0x400000, PROT_READ, MAP_SHARED, fd, 0);
    printf("%c %c %c %c\n",p[0],p[1],p[3],p[4]);
    munmap(p, 0x400000);
    return 0;
}

// transfer fd in processes
static void send_fd(int scoket, int *fd, int n) {
    struct msghdr msg = { 0 };
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n*sizeof(int))];
    char data;
    memset(buf, '\0', sizeof(buf));
    struct iovec iov = { .iov_base = &data, .iov_len = 1 };
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(n*sizeof(int));

    memcpy((int*) CMSG_DATA(cmsg), fd, n*sizeof(int));
    if (sendmsg(scoket, &msg, 0) < 0) {
        printf("Failed to send message\n");
        //handle_error("Failed to send message");
    }
}

//
static int * recv_fd(int scoket, int n) {
    int *fd = (int *) malloc(n * sizeof(int));
    struct msghdr msg = {0};
    struct cmsghdr *cmsg;
    char buf[CMSG_SPACE(n * sizeof(int))], data;
    memset(buf, '\0', sizeof(buf));
    struct iovec iov = {.iov_base = &data, .iov_len = 1};

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(scoket, &msg, 0) < 0) {
        printf("Failed to receive message\n");
        //handle_error("Failed to receive message");
        cmsg = CMSG_FIRSTHDR(&msg);
        memcpy(fd, (int *) CMSG_DATA(cmsg), n * sizeof(int));
        return fd;
    }
}

#endif