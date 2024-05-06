#ifndef _EPOLL_REACTOR_H
#define _EPOLL_REACTOR_H

#include "common.h"

inline int EPOLL_MAX_EVENTS = 4096;

class CEpollReactor;
class CEventHandler {
public:
    CEventHandler(CEpollReactor * pReactor);
    virtual ~CEventHandler() { }

    virtual int HandleInput();

    virtual int HandleOutput();

    void GetIds(int *pReadId, int *pWriteId){}
};

class CEpollReactor  {
public:
    CEpollReactor();
    virtual ~CEpollReactor() { close(m_fdEpoll);}
    void RegisterIO(CEventHandler * pEventHandle);
    void RemoveIO(CEventHandler * pEventHandle);

private:
    void DispatchIOs();
private:
    int m_fdEpoll;
    std::map<CEventHandler *, int> m_mapEvents;
};


#endif