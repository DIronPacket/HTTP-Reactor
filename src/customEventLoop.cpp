#include "customEventLoop.h"

CustomEventLoop::CustomEventLoop() : thread_id(std::this_thread::get_id()),
                                     looping(false),
                                     quit(false),
                                     epoller(new CustomEpoller(this))
{
    std::ostringstream oss;
    oss << thread_id;
    thread_id_str = oss.str();
}
CustomEventLoop::~CustomEventLoop()
{
}
void CustomEventLoop::loop()
{
    assert(!looping);
    assert(thread_id == std::this_thread::get_id());
    looping = true;
    while (!quit)
    {
        LOG(outHead("info") + thread_id_str + "开始准备进入事件循环", false, SUB_REACTOR);
        epoller->poll(kEPollTimeoutMs);
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 休眠100毫秒
    }
    looping = false;
}
void CustomEventLoop::addEventWaitFd(CustomConnector *customConnector)
{
    if (customConnector)
        epoller->addEpollWaitFd(customConnector);
}
void CustomEventLoop::modifyConnector(CustomConnector *customConnector)
{
    if (customConnector)
        epoller->modifyConnector(customConnector);
}

void CustomEventLoop::removeConnector(CustomConnector *customConnector)
{
    if (customConnector)
        epoller->removeFd(customConnector);
}
int CustomEventLoop::HandleRead(CustomConnector *customConnector)
{
    if (!customConnector)
        return -1;
    HttpRequest *temp_request = customConnector->httpRequest;
    temp_request->currentState = HANDLE_INIT;
    int recvLen = 0;
    while (1)
    {
        // 循环接收数据，直到缓冲区读取不到数据或请求消息处理完成时退出循环
        // std::cout<<"准备接收数据"<<std::endl;
        recvLen = recv(customConnector->connectFd, customConnector->inputBuffer, 2048, 0);
        // std::cout << "当前读取数据量：" << std::endl
        //           << customConnector->inputBuffer << std::endl;

        // 对方关闭连接，直接断开连接，设置当前状态为 HANDLE_ERROR，再退出循环
        if (recvLen == 0)
        {
            temp_request->currentState = HANDLE_ERROR;
            LOG(outHead("error") + thread_id_str + "对方关闭连接，当前直接断开连接！", true, ERROR_LOG);
            break;
        }
        // 如果缓冲区的数据已经读完，退出读数据的状态
        else if (recvLen == -1)
        {
            if (errno != EAGAIN)
            { // 如果不是缓冲区为空，设置状态为错误，并退出循环
                temp_request->currentState = HANDLE_ERROR;
                LOG(outHead("error") + thread_id_str + "数据已经读完，但是缓冲区还有数据，读取错误！", true, ERROR_LOG);
                break;
            }
            else if (errno == EAGAIN)
            {
                LOG(outHead("info") + thread_id_str + "系统调用错误，continue继续读取数据！", false, SUB_REACTOR);
                continue;
            }
            // 如果是缓冲区为空，表示需要等待数据发送，由于是 EPOLLONESHOT，再退出循环，等再发来数据时再来处理
            modifyWaitFd(epoller->epfd, customConnector->connectFd, true, true, false);
            LOG(outHead("info") + thread_id_str + "缓冲区为空，数据尚未读完， continue 继续 errno:" + std::to_string(errno), true, SUB_REACTOR);
            continue;
        }

        temp_request->recvMsg.append(customConnector->inputBuffer, recvLen);
        std::string::size_type endIndex = 0;
        // 如果是初始状态，获取请求行
        if (temp_request->currentState == HANDLE_INIT)
        {
            endIndex = temp_request->recvMsg.find("\r\n"); // 查找请求行的结束边界
            if (endIndex != std::string::npos)
            {
                // 记录请求行信息
                temp_request->setRequestLine(temp_request->recvMsg.substr(0, endIndex + 2));
                // requestStatus[m_clientFd].setRequestLine(requestStatus[m_clientFd].recvMsg.substr(0, endIndex + 2));
                // 删除请求行部分的缓存
                temp_request->recvMsg.erase(0, endIndex + 2);
                // 将状态设置为处理请求头
                temp_request->currentState = HANDLE_HEAD;
            }
            // 如果没有找到 \r\n，表示数据还没有接收完成，会跳回上面继续接收数据
        }
        // 如果是处理首部的状态，逐行解析首部字段，直至遇到空行
        if (temp_request->currentState == HANDLE_HEAD)
        {
            // 用于暂存获取的一行请求头数据
            std::string curLine;
            while (1)
            {
                // 获取一行的边界
                endIndex = temp_request->recvMsg.find("\r\n");
                // 没有找到边界，退出内层循环，等待下一次接收数据
                if (endIndex == std::string::npos)
                {
                    break;
                }
                // 将该行的内容取出
                curLine = temp_request->recvMsg.substr(0, endIndex + 2);
                // 删除收到的数据中的该行数据
                temp_request->recvMsg.erase(0, endIndex + 2);
                // 如果是空行，将状态修改为等待解析请求体
                if (curLine == "\r\n")
                {
                    temp_request->currentState = HANDLE_BODY;
                    // 退出首部字段循环
                    break;
                }
                // 如果不是空行，需要解析字符串将该首部保存
                temp_request->addHeaderOpt(curLine);
                // std::cout<<"解析消息头："<<curLine<<std::endl;
            }
        }
        if (temp_request->currentState == HANDLE_BODY)
        {
            // std::cout << "解析消息体：" << temp_request->recvMsg.size() << "     " << temp_request->contentLength << std::endl;
            if (temp_request->recvMsg.size() < temp_request->contentLength)
            {
                // std::cout << "消息体接收不完全" << std::endl;
                continue;
            }

            temp_request->body = temp_request->recvMsg;
            // std::cout<<"当前缓存request消息体："<<temp_request->body<<std::endl;
            temp_request->recvMsg.clear();
            // 提交到工作线程池
            temp_request->currentState = HADNLE_COMPLATE;
            LOG(outHead("info") + thread_id_str + "接收客户端数据完毕，退出循环！", true, SUB_REACTOR);
            break;
        }
    }

    // 如果处理状态错误
    if (temp_request->currentState == HANDLE_ERROR)
        return -1;
    LOG(outHead("info") + thread_id_str + "HTTP消息接收并处理完毕！", false, SUB_REACTOR);
    temp_request->currentState = HADNLE_COMPLATE;
    // 提交给工作线程池
    customConnector->myserver->workThreadPool->submit(customConnector);
    // 禁止读事件
    customConnector->disableRead();
    return 0;
}

int CustomEventLoop::HandleWrite(CustomConnector *customConnector)
{
    if (!customConnector)
        return -1;
    CustomBuffer *outputBuffer = customConnector->outputBuffer;
    std::string writeData = outputBuffer->data;
    size_t remaining = outputBuffer->writeableSize();
    ssize_t nwrote = 0;
    while (remaining > 0)
    {
        nwrote = send(customConnector->connectFd, writeData.c_str(), remaining, 0);
        if (nwrote > 0)
        {
            // LOG(outHead("info") + thread_id_str + " 向客户端发送数据：\r\n" + writeData.c_str(), true, SUB_REACTOR);
            remaining -= nwrote;
            outputBuffer->writeIndex += nwrote;
        }
        else
        {
            if (nwrote < 0)
            {
                if (errno == EWOULDBLOCK)
                {
                    // 没有更多数据可写，等待下一次写事件触发
                    LOG(outHead("info") + thread_id_str + " 缓冲区满，等待继续发送", false, SUB_REACTOR);
                    continue; // 没有更多数据可写，等待下一次写事件触发
                }
                else
                {
                    LOG(outHead("info") + thread_id_str + " 发送错误，关闭当前连接", false, SUB_REACTOR);
                    return 0;
                }
            }
            // 如果write返回0，对端关闭连接
            if (nwrote == 0)
            {
                LOG(outHead("error") + thread_id_str + " 发送数据时对端关闭连接", false, ERROR_LOG);
                return 0;
            }
        }
    }
    if (customConnector->httpResponse->responseHeaders["Content-Type"] == "application/octet-stream")
    {
        off_t offset = 0; // 初始化偏移量
        while (offset < customConnector->httpResponse->fileSize)
        {
            nwrote = sendfile(customConnector->connectFd, customConnector->httpResponse->fileMsgFd, &offset, customConnector->httpResponse->fileSize - offset);
            if (nwrote > 0)
            {
                LOG(outHead("info") + thread_id_str + " 文件发送成功，发送字符数：" + std::to_string(nwrote), true, SUB_REACTOR);
            }
            else if (nwrote < 0)
            {
                if (errno == EWOULDBLOCK)
                {
                    LOG(outHead("info") + thread_id_str + " 发送文件时发送缓冲区满，等待继续发送", false, SUB_REACTOR);
                    continue; // 没有更多数据可写，等待下一次写事件触发
                }
                else
                {
                    LOG(outHead("error") + thread_id_str + " 发送文件数据时发生错误", false, ERROR_LOG);
                    close(customConnector->httpResponse->fileMsgFd); // 关闭文件描述符
                    return 0;             // 发生错误，关闭连接
                }
            }
            else // nwrote == 0，对端关闭连接
            {
                LOG(outHead("error") + thread_id_str + " 发送文件时对端关闭连接", false, ERROR_LOG);
                close(customConnector->httpResponse->fileMsgFd); // 关闭文件描述符
                return 0;
            }
        }
        close(customConnector->httpResponse->fileMsgFd); // 发送完毕后关闭文件描述符
    }
    customConnector->disableWrite();
    customConnector->httpResponse->reset();
    return nwrote;
}

int CustomEventLoop::HandleClose(CustomConnector *customConnector)
{
    if (!customConnector || customConnector==nullptr)
        return -1;
    // 删除连接信息
    customConnector->deregister();
    delete customConnector;
    customConnector = nullptr;
    LOG(outHead("info") + thread_id_str + " 线程关闭当前连接", false, SUB_REACTOR);
    return 0;
}
void CustomEventLoop::stopEventLoop(CustomEventLoop *customEventLoop)
{
    quit = true;
    LOG(outHead("info") + thread_id_str + " 线程停止当前事件循环", false, SUB_REACTOR);
    customEventLoop->~CustomEventLoop();
}