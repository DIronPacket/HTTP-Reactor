#ifndef EPOLLER_H
#define EPOLLER_H

#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>
#include <errno.h>
#include <signal.h>
#include <atomic>
#include <unistd.h>
#include <vector>
#include <sys/epoll.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include "customConnector.h"
#include "customEventLoop.h"
class CustomEventLoop;
class CustomConnector;
typedef std::vector<epoll_event> EventList;
typedef std::map<int, CustomConnector *> ConnectorMap;

class CustomEpoller
{
public:
    CustomEpoller(CustomEventLoop *eventLoop);
    ~CustomEpoller();
    void poll(int timeout);
    void addEpollWaitFd(CustomConnector *customConnector);
    void modifyConnector(CustomConnector *customConnector);
    void removeFd(CustomConnector *customConnector);

public:
    std::mutex map_mutex;
    ConnectorMap connectorMap;
    CustomEventLoop *owner_loop;
    int epfd;
private:
    EventList revents;
    static const int InitEventsSize = 100;
};

#endif