#include "myserver.h"

int MyServer::m_epollfd = -1;
bool MyServer::isRunning = false;

MyServer::MyServer(int port_, int thread_num,std::shared_ptr<CustomConfig> customConfig_) : port(port_),
                                                customConfig(customConfig_),
                                                io_mutex(std::make_shared<ReadWriteLock>()),
                                                threadNum(thread_num),
                                                acceptor(new Acceptor(port_)),
                                                threadPool(new ReactorThreadPool(thread_num)),
                                                workThreadPool(new WorkThreadPool(thread_num))
{
    // 设置信号处理器
    struct sigaction sa;
    // 设置信号处置程序
    sa.sa_handler = MyServer::signal_handler_wrapper;
    // 初始化信号屏蔽集为空
    sigemptyset(&sa.sa_mask);
    // 设置标志位为0
    sa.sa_flags = 0;
    // 安装新的信号处理程序
    sigaction(SIGINT, &sa, nullptr);
}
MyServer::~MyServer()
{
    int err = close(acceptor->listen_fd);
    if (err == 0)
    {
        LOG(outHead("info") + "监听套接字关闭成功:"+std::to_string(acceptor->listen_fd), false, MAIN_REACTOR);
    }
    else
    {
        LOG(outHead("error") + "监听套接字关闭失败:"+std::to_string(acceptor->listen_fd)+" , err:"+std::to_string(err), true, ERROR_LOG);
    }
    delete acceptor;
    acceptor = nullptr;
}
void MyServer::start()
{
    // 启动线程池
    threadPool->start();
    // 启动主线程循环
    m_epollfd = epoll_create1(0);
    if(m_epollfd==-1)
    {
        LOG(outHead("error") + "主线程创建套接字失败!", true, ERROR_LOG);
        return;
    }
    // ListenFd 设置为 边沿触发、非阻塞
    setNonBlocking(acceptor->listen_fd);
    // 因为需要将连接客户端的任务交给子线程处理，所以设置为边沿触发，避免子线程还没有接受连接时事件一直产生
    int ret = addWaitFd(m_epollfd, acceptor->listen_fd, false, false);
    if (ret != 0)
    {
        LOG(outHead("error") + "添加监控 Listen 套接字失败", false, ERROR_LOG);
        return;
    }
    LOG(outHead("info") + "epoll 中添加监听套接字成功", false, MAIN_REACTOR);
    isRunning = true;
    int res_ = waitEpoll();
    LOG(outHead("info") + "结束程序!", true, MAIN_REACTOR);
    //立即终止当前程序
    exit(0);
}

// 主线程中负责监听所有事件
int MyServer::waitEpoll()
{
    while (isRunning)
    {
        int resNum = epoll_wait(m_epollfd, resEvents, MAX_RESEVENT_SIZE, -1);
        if (resNum < 0 && errno != EINTR)
        {
            LOG(outHead("error") + "epoll_wait 执行错误", true, ERROR_LOG);
            return -1;
        }
        LOG(outHead("info") + "epoll_wait 监听到新事件:" + std::to_string(resNum), false, MAIN_REACTOR);
        for (int i = 0; i < resNum; ++i)
        {
            int resfd = resEvents[i].data.fd;
            // 检查是否是读事件，并且只处理监听套接字上的读事件
            if (resEvents[i].events & EPOLLIN)
            {
                // 检查这个文件描述符是否是监听套接字的文件描述符
                if (resfd == acceptor->listen_fd) // 假设m_listenfd是监听套接字的文件描述符
                {
                    LOG(outHead("info") + "reactor 监听到连接事件", false, MAIN_REACTOR);
                    createConnection();
                }
                else
                {
                    // 如果不是监听套接字，则忽略或记录日志
                    LOG(outHead("error") + "reactor 忽略非监听套接字的EPOLLIN事件", false, ERROR_LOG);
                }
            }
            else
            {
                LOG(outHead("error") + "reactor 监听到其他不支持的事件", false, ERROR_LOG);
            }
        }
    }
    threadPool->stop();
    workThreadPool->stopWork(workThreadPool);
    return 0;
}
void MyServer::createConnection()
{
    int connect_fd = acceptor->acceptClient();
    fcntl(connect_fd, F_SETFL, O_NONBLOCK);
    CustomEventLoop *sub_loop = threadPool->getNextLoop();
    CustomConnector *customConnector = new CustomConnector(connect_fd, sub_loop, this);
    // LOG(outHead("info") + "reactor 成功创建新连接并间接交给子reactor ", true, MAIN_REACTOR);
}

