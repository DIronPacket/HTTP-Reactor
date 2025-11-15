#include "workEvent.h"

int parseFileSaveLocal(HttpResponse *httpResponse, HttpRequest *httpRequest, std::shared_ptr<ReadWriteLock> io_mutex)
{
    int res_ = 0;
    // 查找 \r\n
    int endIndex = httpRequest->body.find("\r\n");
    // 当前状态下，\r\n 前的数据必然是文件信息开始的标志
    if (endIndex != std::string::npos)
    {
        // 截取边界
        std::string flagStr = httpRequest->body.substr(0, endIndex);
        // 判断是否找到边界
        if (flagStr == "--" + httpRequest->requestHeaders["boundary"])
        {
            // 将开始标志行删除（包括 /r/n）
            httpRequest->body.erase(0, endIndex + 2);
        }
        // 没有找到边界
        else
            return -1;
    }
    // 文件接收不完整
    else
        return -1;
    // 从中提取文件名
    std::string strLine;
    // 查找 \r\n 表示一行数据
    endIndex = httpRequest->body.find("\r\n");
    if (endIndex != std::string::npos)
    {
        // 获取这一行的数据信息
        strLine = httpRequest->body.substr(0, endIndex + 2);
        // 删除缓存
        httpRequest->body.erase(0, endIndex + 2);
        // 检测是否为空行，如果是空行，修改状态，退出
        if (strLine == "\r\n")
        {
            SPDLOG_ERROR("文件读取失败！");
            return -1;
        }
        // 查找 strLine 是否包含 filename
        endIndex = strLine.find("filename");
        if (endIndex != std::string::npos)
        {
            // 将真正 filename 前的所有字符删除
            strLine.erase(0, endIndex + std::string("filename=\"").size());
            for (int i = 0; strLine[i] != '\"'; ++i)
            {
                httpRequest->recvFileName += strLine[i];
            }
        }
    }
    // 如果没有找到，表示消息还没有接收完整，退出，等待下一轮的事件中继续处理
    else
    {
        SPDLOG_ERROR("文件读取失败！");
        return -1;
    }

    // 如果处于等待并处消息体中文件内容部分
    // 循环检索是否有 \r\n ，将 \r\n 之前的内容全部保存。如果存在\r\n，根据后面的内容判断是否到达文件边界
    std::size_t pos = httpResponse->resourseName.find_last_of("/\\");
    std::string directoryPath = (pos == std::string::npos) ? "" : httpResponse->resourseName.substr(0, pos);
    // 判断是否存在这个文件夹路径，不存在就创建
    if (!is_exits_path(directoryPath))
    {
        bool is_ok = createDirectories(directoryPath);
        if (!is_ok)
            return -1;
    }
    // 以覆盖写入模式打开文件
    std::ofstream ofs(httpResponse->resourseName, std::ios::out | std::ios::trunc | std::ios::binary);
    if (!ofs.is_open())
    {
        SPDLOG_ERROR("文件打开失败！");
        return -1;
    }
    while (1)
    {
        // 该变量用来保存 根据\r的位置决定向文件中写入多少字符，初始为所有字符长度
        int saveLen = httpRequest->body.size();
        if (saveLen == 0)
        {
            // 长度为空时退出循环，等待接收到数据时再处理
            ofs.close();
            return -1;
        }
        // 在剩余的字符中搜索标志 \r
        endIndex = httpRequest->body.find('\r');

        if (endIndex != std::string::npos)
        {
            // 如果有\r，后面有可能是文件结束标识
            // 首先判断 \r 后的数据是否满足结束标识的长度，是否大于等于 sizeof(\r\n + "--" + boundary + "--" + \r\n)
            int boundarySecLen = httpRequest->requestHeaders["boundary"].size() + 8;
            if (httpRequest->body.size() - endIndex >= boundarySecLen)
            {
                // 判断后面这部分数据是否为结束边界"\r\n"
                if (httpRequest->body.substr(endIndex, boundarySecLen) ==
                    "\r\n--" + httpRequest->requestHeaders["boundary"] + "--\r\n")
                {
                    if (endIndex == 0)
                    {
                        // 表示边界前的数据都已经写入文件，设置文件接收完成，进入下一个状态
                        break;
                    }
                    // 如果后面不是结束标识，先将 \r 之前的所有数据写入文件，在循环的下一轮会进到上一个if，结束整个处理
                    saveLen = endIndex;
                }
                else
                {
                    // 如果不是边界，在 \r 后再次搜索 \r，如果搜索到了，写入的数据截至到第二个 \r，否则将所有数据写入
                    endIndex = httpRequest->body.find('\r', endIndex + 1);
                    if (endIndex != std::string::npos)
                    {
                        saveLen = endIndex;
                    }
                }
            }
            else
            {
                ofs.close();
                return -1;
            }
        }
        // 如果没有退出表示当前仍是数据部分，将 saveLen 字节的数据存入文件，并将这些数据从 recvMsg 数据中删除
        ofs.write(httpRequest->body.c_str(), saveLen);
        httpRequest->body.erase(0, saveLen);
    }
    ofs.close();
    SPDLOG_TRACE("文件保存成功！");
    return 0;
}
