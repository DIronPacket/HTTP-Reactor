#include "reactorThreadPool.h"

ReactorThreadPool::ReactorThreadPool(int threadNum) : m_threadNum(threadNum),
                                                      subLoops(),
                                                      tempLoop(NULL),
                                                      started(false),
                                                      next(0)
{
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    SPDLOG_TRACE("reactor 线程池构造完成！");
}

ReactorThreadPool::~ReactorThreadPool()
{

}
void ReactorThreadPool::start()
{
    SPDLOG_TRACE("reactor 线程池开始启动，初始化线程池数量：{}",std::to_string(m_threadNum));
    for (int i = 0; i < m_threadNum; i++)
    {
        std::thread t(&ReactorThreadPool::eventLoopThreadRun, this);
        pthread_mutex_lock(&mutex);
        while (tempLoop == NULL)
        {
            pthread_cond_wait(&cond, &mutex);
        }
        pthread_mutex_unlock(&mutex);

        subLoops.push_back(tempLoop);
        tempLoop = NULL;
        t.detach();
    }

    started = true;
    SPDLOG_TRACE("reactor 线程池已经启动");
}
void ReactorThreadPool::stop()
{
    for (int i = 0; i < m_threadNum; ++i)
    {
        CustomEventLoop *cur_loop = subLoops[i];
        cur_loop->stopEventLoop(cur_loop);
        SPDLOG_TRACE("reactor 从子线程池中修改子线程状态：{}",cur_loop->thread_id_str);
    }
}
CustomEventLoop *ReactorThreadPool::getNextLoop()
{
    assert(started);
    CustomEventLoop *selected;
    if (m_threadNum > 0)
    {
        selected = subLoops[next];
        next++;
        if (next >= m_threadNum)
        {
            next = 0;
        }
        SPDLOG_TRACE("reactor 从子线程池中拿到一个子线程");
    }
    if (selected == nullptr)
    {
        SPDLOG_ERROR("reactor 分发连接时没有拿到子线程");
    }
    return selected;
}
void ReactorThreadPool::eventLoopThreadRun()
{
    CustomEventLoop *eventLoop = new CustomEventLoop();
    pthread_mutex_lock(&mutex);
    tempLoop = eventLoop;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex);
    SPDLOG_TRACE("初始化子线程 {}",eventLoop->thread_id_str);
    eventLoop->loop();
}
