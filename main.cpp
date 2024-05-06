#include "common.h"
//#include "demo.h"
//#include "linux_communicator/io_pipe.h"
//#include "linux_communicator/io_shared_memory.h"

#include "linux_server/cpt_11_2_2.h"

int main(int argc, char * argv[]) {
    /////////// test linux IO
    //test_pipe();
    //test_name_pipe(argc, argv);

    /////////// test shared memory
    //test_shm_write();
    //test_shm_read();

    ////////// test epoll
    //readout_LT();
    //Write_ET_hang();
    //Write_LT_loop();
    //pure_ET();
    //Reset_ET();
    //Write_ET_loop();

    ///////// test chapter 11_2_2.h
    n_timer_server::test_timer_server(argc, argv);

    return 0;
}


//经过前面的案例分析，我们已经了解到，当epoll工作在ET模式下时，对于读操作，如果read一次没有读尽buffer中的数据，那么下次将得不到读就绪的通知，造成buffer中已有的数据无机会读出，除非有新的数据再次到达。对于写操作，主要是因为ET模式下fd通常为非阻塞造成的一个问题——如何保证将用户要求写的数据写完。
//要解决上述两个ET模式下的读写问题，我们必须实现：
//1. 对于读，只要buffer中还有数据就一直读；
//2. 对于写，只要buffer还有空间且用户请求写的数据还未写完，就一直写。

//ET模式下的accept问题
//请思考以下一种场景：在某一时刻，有多个连接同时到达，服务器的 TCP 就绪队列瞬间积累多个就绪连接，由于是边缘触发模式，epoll 只会通知一次，accept 只处理一个连接，导致 TCP 就绪队列中剩下的连接都得不到处理。在这种情形下，我们应该如何有效的处理呢？

//解决的方法是：解决办法是用 while 循环抱住 accept 调用，处理完 TCP 就绪队列中的所有连接后再退出循环。如何知道是否处理完就绪队列中的所有连接呢？ accept  返回 -1 并且 errno 设置为 EAGAIN 就表示所有连接都处理完。

//关于ET的accept问题，这篇博文的参考价值很高，如果有兴趣，可以链接过去围观一下。