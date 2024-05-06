#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class tw_timer;
// 绑定socket和定时器
struct client_data{
    sockaddr_in address;
    int sockfd;
    char buf[BUFFER_SIZE];
    tw_timer* timer;
};

// 定时器类
class tw_timer{
public:
    tw_timer(int rot, int ts)
            :next(nullptr),prev(nullptr),rotation(rot), time_slot(ts){}

public:
    int rotation;       // 记录定时器在时间轮转多少圈后生效【轮次】
    int time_slot;      // 记录定时器数据时间轮上哪个槽
    void (*cb_func)(client_data*);      // 定时器回调函数
    client_data* user_data;         // 客户数据
    tw_timer* next;             // 下一个定时器
    tw_timer* prev;             // 前一个定时器
};

class time_wheel{
public:
    // 初始化每个槽的头节点
    time_wheel():cur_slot(0){
        for(int i=0;i<N;i++){
            slots[i] = nullptr;
        }
    }
    // 遍历每个槽，并销毁定时器
    ~time_wheel(){
        for(int i=0;i<N;i++){
            tw_timer* tmp = slots[i];
            while(tmp){
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }

    // 根据定时器值timeout创建一个定时器，并插入到合适的槽中
    tw_timer* add_timer(int timeout){
        if(timeout < 0){
            return nullptr;
        }
        int ticks = 0;
        // 根据定时器超时值计算其在时间轮多少个抵达后被触发
        // 将滴答数存在变量ticks中
        if(timeout < SI){
            ticks = 1;
        } else {
            ticks = timeout/SI;
        }
        // 计算待插入的定时器在多少圈后被触发
        int rotation = ticks/N;
        // 计算待插入的定时器应该被插入到哪个槽中
        int ts = (cur_slot+ (ticks % N) ) %N;
        // 创建新的定时器，其在时间轮转动rotation后被触发，位于第ts个槽上
        tw_timer * timer = new tw_timer(rotation, ts);
        // 如果ts槽上无任何定时器，则将其设为头节点
        if(!slots[ts]){
            printf("add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot);
            slots[ts] = timer;
        }
            // 否则插入第ts槽中
        else{
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }

    // 删除目标定时器
    void del_timer(tw_timer* timer){
        if(!timer){
            return;
        }
        int ts = timer->time_slot;
        // slots[ts]是目标定时器所在的头节点
        if(timer == slots[ts]){
            slots[ts] = slots[ts]->next;
            if(slots[ts]){
                slots[ts]->prev = nullptr;
            }
            delete timer;
        } else {
            timer->prev->next = timer->next;
            if(timer->next){
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }

    // SI时间到后，调用该函数，时间轮向前滚动一个槽的间隔
    void tick(){
        tw_timer* tmp = slots[cur_slot];
        printf("current slot is %d\n", cur_slot);
        while(tmp){
            printf("tick the timer once\n");
            // 如果定时器的rotation值大于0，则它在这一轮不起作用
            if(tmp->rotation>0){
                tmp->rotation--;
                tmp = tmp->next;
            }
                // 否则执行任务，删除定时器
            else{
                tmp->cb_func(tmp->user_data);
                if(tmp == slots[cur_slot]){
                    printf("delete header in cur_slot\n");
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if(slots[cur_slot]){
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                } else {
                    tmp->prev->next = tmp->next;
                    if(tmp->next){
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        cur_slot = ++cur_slot % N;      // 更新时间轮的当前槽
    }
private:
    static const int N = 60;        // 时间轮上的槽数
    static const int SI = 1;        // 每1s转动一次，即槽间隔为1s
    tw_timer* slots[N];             // 时间轮的槽，每个元素指向一个定时器链表，链表无序
    int cur_slot;                   // 时间轮的当前槽
};

#endif // !TIME_WHEEL_TIMER