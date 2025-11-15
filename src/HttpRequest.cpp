#include "HttpRequest.h"

void HttpRequest::reset()
{
    if (!version.empty())
        version.clear();
    if (!method.empty())
        method.clear();
    if (!url.empty())
        url.clear();
    currentState = HANDLE_INIT;
    if (!requestHeaders.empty())
        requestHeaders.clear();
}

bool HttpRequest::keepAlive()
{
    if (requestHeaders.find("Connection") != requestHeaders.end() && requestHeaders["Connection"] == "close")
    {
        return false;
    }
    return true;
}
int HttpRequest::setRequestLine(const std::string &requestLine)
{
    std::istringstream lineStream(requestLine);
    lineStream >> method;
    lineStream >> url;
    lineStream >> version;
    if (method.empty() || url.empty() || version.empty())
        return -1;
    return 0;
}
void HttpRequest::addHeaderOpt(const std::string &headLine)
{
    std::istringstream lineStream;
    lineStream.str(headLine); // 以 istringstream 的方式处理头部选项

    std::string key, value; // 保存键和值的临时量

    lineStream >> key; // 获取 key
    key.pop_back();    // 删除键中的冒号
    lineStream.get();  // 删除冒号后的空格

    // 读取空格之后所有的数据，遇到 \n 停止，所以 value 中还包含一个 \r
    getline(lineStream, value);
    value.pop_back(); // 删除其中的 \r

    if (key == "Content-Length")
    {
        // 保存消息体的长度
        contentLength = std::stoll(value);
        SPDLOG_TRACE("当前消息体的长度：{}",std::to_string(contentLength));
    }
    else if (key == "Content-Type")
    {
        // 分离消息体类型。消息体类型可能是复杂的消息体，类似 Content-Type: multipart/form-data; boundary=---------------------------24436669372671144761803083960
        // 先找出值中分号的位置
        size_t semIndex = value.find(';');
        // 根据分号查找的结果，保存类型的结果
        if (semIndex != std::string::npos)
        {
            requestHeaders[key] = value.substr(0, semIndex);
            size_t eqIndex = value.find('=', semIndex);
            // std::string::size_type
            if (eqIndex != std::string::npos && eqIndex > semIndex) // 确保等号在分号之后
            {
                std::string paramKey = value.substr(semIndex + 1, eqIndex - semIndex - 1); // 参数键
                requestHeaders[paramKey] = value.substr(eqIndex + 1);                      // 参数值
            }
            else
            {
                // 没有找到等号，或者等号位置不正确
                requestHeaders[key] = value.substr(semIndex + 1); // 将剩余部分作为值
            }
            // key = value.substr(semIndex + 1, eqIndex - semIndex - 1);
            // requestHeaders[key] = value.substr(eqIndex + 1);
            // std::cout<<"-----------"<<key<<std::endl<<"--------"<<msgHeader[key]<<std::endl;
        }
        else
        {
            requestHeaders[key] = value;
        }
    }
    else
    {
        requestHeaders[key] = value;
    }
}