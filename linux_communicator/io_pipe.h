#ifndef __IO_PIPE_H_
#define __IO_PIPE_H_

#include "../common.h"

int test_pipe() {
    int i=0;
    int result = -1;
    int fd[2],nbytes;
    char string[100];
    char readbuffer[80];
    memset(readbuffer, 0, sizeof(readbuffer));
    int *write_fd = &fd[1];
    int *read_fd = &fd[0];
    printf("Please input data:");
    scanf("%s",string);
    result = pipe(fd);
    if(-1 == result)
    {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if(-1 == pid) //此处为了验证父子进程是否创建成功，如果未创建成功，则返回-1
    {
        perror("fork");
        return -1;
    }
    else if(0 == pid)
    {
        printf("child process %d\n", getpid());
        close(*read_fd);
        result = write(*write_fd,string,strlen(string));
        return 0;
    }
    else
    {
        wait(nullptr);
        printf("parent process %d\n", getpid());
        close(*write_fd);
        nbytes = read(*read_fd,readbuffer,sizeof(readbuffer)-1);
        printf("receive %d data：%s\n",nbytes,readbuffer);
    }
    return 0;
}


int test_name_pipe(int argc, char *argv[]) {
    char buffer[1024] = "hello";
    if (argc < 2) {
        printf("./a.out fifoname\n");
        exit(1);
    }
    int fd = open(argv[1],O_WRONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }
    //
    auto flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
    //
    write(fd,buffer,sizeof(buffer));
    close(fd);
    return 0;
}


#endif