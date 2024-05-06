#ifndef _CPT_11_4_1_H_
#define _CPT_11_4_1_H_

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class tw_timer;

struct tw_client_data {
    sockaddr_in _address;
    int _sockfd;
    char _buf[BUFFER_SIZE];
    tw_timer * _timer;
};

class tw_timer {
public:
    tw_timer(int rot, int ts) : _next(nullptr), _prev(nullptr), _rotation(rot), _time_slot(ts) {}
public:
    int _rotation;
    int _time_slot;
    void (*cb_func)(tw_client_data *);
    tw_client_data * _user_data;
    tw_timer * _prev;
    tw_timer * _next;
};

class time_wheel {
public:
    time_wheel() : _cur_slot(0) {
        for (int i = 0; i < N; ++i) {
            _slots[i] = nullptr;
        }
    }
    ~time_wheel() {
        for (int i = 0; i < N; ++i) {
            tw_timer * tmp = _slots[i];
            while (tmp) {
                _slots[i] = tmp->_next;
                delete tmp;
                tmp = _slots[i];
            }
        }
    }

    tw_timer * add_timer(int timeout) {
        if (timeout < 0) {
            return nullptr;
        }
        int ticks = 0;
        if (timeout < SI) {
            ticks = 1;
        } else {
            ticks = timeout / SI;
        }
        int rotation = ticks/N;
        int ts = (_cur_slot + (ticks%N)) % N;
        tw_timer * _timer = new tw_timer(rotation, ts);
        if (!_slots[ts]) {
            printf("add timer\n");
            _slots[ts] = _timer;
        } else {
            _timer->_next = _slots[ts];
            _slots[ts]->_prev = _timer;
            _slots[ts] = _timer;
        }
    }

    void del_timer(tw_timer * _timer) {
        if(!_timer) {
            return;
        }
        int ts = _timer->_time_slot;
        if (_timer == _slots[ts]) {
            _slots[ts] = _slots[ts]->_next;
            if (_slots[ts]) {
                _slots[ts]->_prev = nullptr;
            }
            delete _timer;
        } else {
            _timer->_prev->_next = _timer->_next;
            if (_timer->_next) {
                _timer->_next->_prev = _timer->_prev;
            }
            delete _timer;
        }
    }

    void tick() {
        tw_timer * tmp = _slots[_cur_slot];
        printf("current slot is %d\n", _cur_slot);
        while (tmp) {
            printf("tick the timer once\n");
            if (tmp->_rotation > 0) {
                tmp->_rotation--;
                tmp = tmp->_next;
            } else {
                tmp->cb_func(tmp->_user_data);
                if (tmp == _slots[_cur_slot]) {
                    printf("delete header in cur_slot\n");
                    _slots[_cur_slot] = tmp->_next;
                    delete tmp;
                    if (_slots[_cur_slot]) {
                        _slots[_cur_slot]->_prev = nullptr;
                    }
                    tmp = _slots[_cur_slot];
                } else {
                    tmp->_prev->_next = tmp->_next;
                    if (tmp->_next) {
                        tmp->_next->_prev = tmp->_prev;
                    }
                    tw_timer * tmp2 = tmp->_next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        _cur_slot = ++_cur_slot % N;
    }

private:
    static const int N = 60;
    static const int SI = 1;
    tw_timer * _slots[N];
    int _cur_slot;
};

#endif