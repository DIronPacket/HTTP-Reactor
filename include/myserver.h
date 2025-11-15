#ifndef WEBSERVER_H
#define WEBSERVER_H
#pragma once
#include <signal.h>
#include <sys/types.h>
#include <stdexcept>
#include <errno.h>
#include <signal.h>
#include <atomic>
#include <unistd.h>
#include "acceptor.h"
#include "customEventLoop.h"
#include "reactorThreadPool.h"
#include "customConnector.h"
#include "workThreadPool.h"
#include "customConfig.h"
class ReactorThreadPool;
class WorkThreadPool;
class CustomEventLoop;

#define MAX_RESEVENT_SIZE 1024 // 事件的最大个数
class MyServer
{
public:
    MyServer(int port, int thread_num,std::shared_ptr<CustomConfig> customConfig_);
    ~MyServer();

    void start();
    // 主线程中负责监听所有事件
    int waitEpoll();
    void createConnection();
    static void signal_handler(int signal)
    {
        // 处理信号的代码
        isRunning = false;
        SPDLOG_INFO("接收到关闭信号");
    }

    static void signal_handler_wrapper(int signal)
    { 
        // 信号处理程序接收信号
        signal_handler(signal);
    }

public:
    ReactorThreadPool *threadPool;
    Acceptor *acceptor;
    WorkThreadPool *workThreadPool;

    std::shared_ptr<CustomConfig> customConfig;
    std::shared_ptr<ReadWriteLock> io_mutex;

private:
    int port;
    int threadNum;
    static int m_epollfd;  // I/O 复用的 epoll 例程文件描述符
    static bool isRunning; // 是否暂停服务器

    epoll_event resEvents[MAX_RESEVENT_SIZE]; // 保存 epoll_wait 结果的数组
};

#endif