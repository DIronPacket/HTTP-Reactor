#include "workThreadPool.h"

WorkThreadPool::WorkThreadPool(int thread_num) : threadNum(thread_num),
                                                 stop(false)
{
    for (int i = 0; i < threadNum; i++)
    {
        threads.emplace_back([this]()
                             {
            while (!this->stop) {
                
                CustomConnector *connector_task;
                {
                    std::unique_lock<std::mutex> lock(this->mutex);
                    while (this->task_queue.empty() && !this->stop) //必须是 while，避免假唤醒
                    {
                        this->cond.wait(lock);
                    }
                    // this->cond.wait(lock, [this](){return this->stop || !this->task_queue.empty();}); //这种方法与上面这种方向效果类似
                    if (this->stop && this->task_queue.empty()) {
                        return;
                    }
                    connector_task = std::move(this->task_queue.front()); //为什么要 std::move？
                    this->task_queue.pop();
                }
                if(connector_task)
                {
                    onMessage(connector_task);
                }
            } });
    }
}
WorkThreadPool::~WorkThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop = true;
    }
    cond.notify_all();
    for (int i = 0; i < threadNum; i++)
    {
        if (threads[i].joinable())
        {
            threads[i].join();
        }
    }
}
void WorkThreadPool::submit(CustomConnector *customConnector)
{
    if (!customConnector)
    {
        SPDLOG_ERROR("提交的业务指针为空指针");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        // Assuming task_queue can hold CustomConnector pointers
        task_queue.push(customConnector);
    }
    cond.notify_one(); // Notify one thread to process the task
}
void WorkThreadPool::stopWork(WorkThreadPool *workThreadPool)
{
    workThreadPool->~WorkThreadPool();
}
int WorkThreadPool::onMessage(CustomConnector *customConnector)
{
    if (!customConnector)
    {
        SPDLOG_ERROR("连接为空指针");
        return -1;
    }
    std::thread::id current_id = std::this_thread::get_id();
    customConnector->httpResponse->keepAlive = customConnector->httpRequest->keepAlive();
    if (customConnector->httpRequest->currentState != HADNLE_COMPLATE)
    {
        return -1;
    }
    int res_ = 0;
    try
    {
        res_ = onRequest(customConnector);
    }
    catch(const exception &e)
    {
        SPDLOG_ERROR("服务器处理报错：{}",e.what());
        res_=-1;
    }

    if (res_ != 0)
    {
        handleFailedRequest(customConnector, res_);
        SPDLOG_ERROR("当前工作线程事件处理失败：{}",customConnector->httpResponse->resourseName);
    }
    customConnector->encodeResponse();
    // 启用写事件
    customConnector->enableWrite();
    customConnector->httpRequest->reset(); // reset for next request under keep alive
    return 0;
}
int WorkThreadPool::onRequest(CustomConnector *customConnector)
{
    HttpRequest *httpRequest = customConnector->httpRequest;
    HttpResponse *httpResponse = customConnector->httpResponse;
    MyServer *temp_server = customConnector->myserver;
    std::string content_type = httpRequest->requestHeaders["Content-Type"];
    std::string path = httpRequest->url;
    httpResponse->resourseName = path;
    int res_ = 0;
    if (httpRequest->method == "GET")
    {
        if (content_type == "application/json")
        {
            // 获取数据
            if (res_ != 0)
            {
                SPDLOG_ERROR("工作线程获取数据失败：{}",customConnector->httpRequest->rquestResourse);
                return res_;
            }
        }
        else if (content_type == "multipart/form-data")
        {
            //获取文件数据
        }
        httpResponse->statusCode = OK;
        httpResponse->statusMessage = "OK";
        httpResponse->body = "200";
        httpResponse->responseHeaders["Content-Type"] = content_type;
    }
    else if (httpRequest->method == "POST")
    {
        // 如果POST请求中没有获取到消息体
        if (httpRequest->body.empty())
        {
            SPDLOG_ERROR("当前工作线程获取到的消息体为空");
            return 1;
        }
        SPDLOG_TRACE("工作线程开始处理POST请求");
        if (content_type == "application/json")
        {
            json recv_j = json::parse(httpRequest->body);
            SPDLOG_TRACE("准备将消息体解析为JSON数据:{}",recv_j.dump());
            //POST业务处理
            httpResponse->responseHeaders["Content-Type"] = content_type;
        }
        // POST中发送的是文件数据
        else if (content_type == "multipart/form-data")
        {
            httpResponse->responseHeaders["Content-Type"] = "application/json";
            // 存储文件数据
            res_ = parseFileSaveLocal(httpResponse, httpRequest, customConnector->myserver->io_mutex);
            if (res_ != 0)
            {
                SPDLOG_ERROR("工作线程保存文件时失败");
                return res_;
            }
            httpResponse->statusCode = OK;
            httpResponse->statusMessage = "OK";
            httpResponse->body = "200";
            SPDLOG_TRACE("工作线程成功获取文件数据");
        }
        // 服务端不支持的数据类型
        else
        {
            SPDLOG_ERROR("服务端不支持的数据类型");
            return 1;
        }
    }
    SPDLOG_TRACE("工作线程处理业务成功");
    return 0;
}
void WorkThreadPool::handleFailedRequest(CustomConnector *customConnector, int result)
{
    HttpResponse *httpResponse = customConnector->httpResponse;
    httpResponse->responseHeaders["Content-Type"] = "application/json";
    if (result == -1)
    {
        httpResponse->statusCode = InternalServerError;
        httpResponse->statusMessage = "InternalServerError";
        httpResponse->body = "500";
    }
    else if (result == 1)
    {
        httpResponse->statusCode = BadRequest;
        httpResponse->statusMessage = "BadRequest";
        httpResponse->body = "400";
    }
    else if (result == 2)
    {
        httpResponse->statusCode = NotFound;
        httpResponse->statusMessage = "NotFound";
        httpResponse->body = "404";
    }
}
