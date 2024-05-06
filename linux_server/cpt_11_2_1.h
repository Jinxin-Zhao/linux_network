#ifndef _CPT_11_2_1_H_
#define _CPT_11_2_1_H_

#include "../common.h"

// 本文件定义了一个升序定时器链表
// sorted list (ascending order)
constexpr std::size_t BUFFER_SIZE = 64;

class Util_timer;
struct ClientData {
    sockaddr_in _address;
    int _sockfd;
    char _buf[BUFFER_SIZE];
    Util_timer * _timer;
};

class Util_timer {
public:
    Util_timer() : _prev(nullptr), _next(nullptr) {}
public:
    time_t _expire;
    std::function<void (ClientData *)> m_callback;
    ClientData * _user_data;
    Util_timer * _prev;
    Util_timer * _next;
};

class Sort_timer_list {
public:
    Sort_timer_list(): _head(nullptr), _tail(nullptr) {}
    ~Sort_timer_list() {
        Util_timer * tmp = _head;
        while (tmp) {
            _head = tmp->_next;
            delete tmp;
            tmp = _head;
        }
    }

    void add_timer(Util_timer * timer) {
        if (!timer) return;
        if (!_head) {
            _head = _tail = timer;
            return;
        }
        if (timer->_expire < _head->_expire) {
            timer->_next = _head;
            _head->_prev = timer;
            _head = timer;
            return;
        }
        add_timer(timer, _head);
    }

    void add_timer(Util_timer * timer, Util_timer * lst_head) {
        Util_timer * prev = lst_head;
        Util_timer * tmp = prev->_next;
        while (tmp) {
            if (timer->_expire < tmp->_expire) {
                prev->_next = timer;
                timer->_prev = prev;
                timer->_next = tmp;
                tmp->_prev = timer;
                break;
            }
            prev = tmp;
            tmp = tmp->_next;
        }
        if (!tmp) {
            prev->_next = timer;
            timer->_prev = prev;
            timer->_next = nullptr;
            _tail = timer;
        }
    }

    void tick() {
        if (!_head) {
            return;
        }
        cout << "timer tick" << endl;
        time_t cur = time(nullptr);
        Util_timer * tmp = _head;
        while (tmp) {
            if (cur < tmp->_expire) {
                break;
            }
            tmp->m_callback(tmp->_user_data);
            _head = tmp->_next;
            if (_head) {
                _head->_prev = nullptr;
            }
            delete tmp;
            tmp = _head;
        }
    }

    void adjust_timer(Util_timer * timer) {
        if (!timer) {
            return ;
        }
        Util_timer * tmp = timer->_next;
        if (!tmp || (timer->_expire < tmp->_expire)) {
            return;
        }
        if (timer == _head) {
            _head = _head->_next;
            _head->_prev = nullptr;
            timer->_next = nullptr;
            add_timer(timer,_head);
        } else {
            timer->_prev->_next = timer->_next;
            timer->_next->_prev = timer->_prev;
            add_timer(timer,timer->_next);
        }
    }
    void del_timer(Util_timer * timer) {
        if (!timer) {
            return;
        }
        if ((timer == _head) && (timer == _tail)) {
            delete timer;
            _head = _tail = nullptr;
            return;
        }
        if (timer == _head) {
            _head = _head->_next;
            _head->_prev = nullptr;
            delete timer;
            return;
        }
        if (timer == _tail) {
            _tail = _tail->_prev;
            _tail->_next = nullptr;
            delete timer;
            return;
        }
        timer->_prev->_next = timer->_next;
        timer->_next->_prev = timer->_prev;
        delete timer;
    }
private:
    Util_timer * _head;
    Util_timer * _tail;
};


#endif