#include "epoll_reactor.h"

CEpollReactor::CEpollReactor() {
    m_fdEpoll = epoll_create(EPOLL_MAX_EVENTS);
}

void CEpollReactor::RegisterIO(CEventHandler *pEventHandle) {
    int nReadID = 0, nWriteID = 0;
    pEventHandle->GetIds(&nReadID,&nWriteID);
    if (nWriteID != 0 && nReadID == 0){
        nReadID = nWriteID;
    }
    if (nReadID != 0) {
        m_mapEvents[pEventHandle] = nReadID;
        epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.ptr = pEventHandle;
        auto res = epoll_ctl(m_fdEpoll, EPOLL_CTL_ADD, nReadID, &ev);
        if (res != 0) {
            printf("epoll_ctl error ,fd[%d], errorNo[%d], %s\n",nReadID,errno,strerror(errno));
        }
    }
}

void CEpollReactor::RemoveIO(CEventHandler *pEventHandle) {
    auto it = m_mapEvents.find(pEventHandle);
    if (it != m_mapEvents.end()) {
        struct epoll_event ev;
        epoll_ctl(m_fdEpoll, EPOLL_CTL_DEL,(*it).second, &ev);
        m_mapEvents.erase(it);
    }
}

void CEpollReactor::DispatchIOs() {
    unsigned int epollTimeout = 10000;
//    if (HandleOtherTask()) {
//        epollTimeout = 0;
//    }
    auto itor = m_mapEvents.begin();
    for (; itor != m_mapEvents.end(); itor++) {
        epoll_event ev;
        auto * pEventHandler = itor->first;
        if (pEventHandler == nullptr){
            continue;
        }
        ev.data.ptr = pEventHandler;
        ev.events = 0;
        int nReadID = 0, nWriteID = 0;
        pEventHandler->GetIds(&nReadID, &nWriteID);
        if (nReadID > 0) {
            ev.events |= EPOLLIN;
        }
        if (nWriteID > 0) {
            ev.events |= EPOLLOUT;
        }
        epoll_ctl(m_fdEpoll, EPOLL_CTL_MOD, itor->second,&ev);
    }

    struct epoll_event events[EPOLL_MAX_EVENTS];
    int nfds = epoll_wait(m_fdEpoll,events, EPOLL_MAX_EVENTS,epollTimeout/1000);
    for (int i = 0; i < nfds; i++) {
        epoll_event & evRef = events[i];
        CEventHandler * pEventHandler = (CEventHandler*)evRef.data.ptr;
        if ((evRef.events | EPOLLIN) != 0 && m_mapEvents.find(pEventHandler) != m_mapEvents.end()) {
            pEventHandler->HandleInput();
        }
        if ((evRef.events | EPOLLOUT) != 0 && m_mapEvents.find(pEventHandler) != m_mapEvents.end()) {
            pEventHandler->HandleOutput();
        }
    }
}

