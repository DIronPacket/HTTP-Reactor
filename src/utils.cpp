#include "utils.h"

void getDateToFileName(std::string &file_name)
{
    // 获取当前系统时间
    std::time_t now = std::time(nullptr);
    // 使用 localtime_s 或 localtime 函数将时间转换为本地时间
    std::tm *local_tm = std::localtime(&now);
    // 从时间结构体中提取日期信息
    int year = local_tm->tm_year + 1900; // 年份，tm_year 从 1900 开始计数
    int month = local_tm->tm_mon + 1;    // 月份，从 0 开始计数，所以要加 1
    int day = local_tm->tm_mday;         // 日
    std::string cur_month, cur_day;
    if (month < 10)
        cur_month = "0" + std::to_string(month);
    else
        cur_month = std::to_string(month);
    if (day < 10)
        cur_day = "0" + std::to_string(day);
    else
        cur_day = std::to_string(day);
    std::string cur_date = std::to_string(year) + cur_month + cur_day;
    file_name = cur_date + ".txt";
}
std::vector<std::string> split(const std::string &str, const std::string &delimiter)
{
    std::vector<std::string> tokens;
    size_t start = 0, end = str.find(delimiter, start);
    while (end != std::string::npos)
    {
        // 提取从 start 到 end（不包括）的子字符串
        tokens.push_back(str.substr(start, end - start));
        // 更新 start 为 delimiter 之后的位置
        start = end + delimiter.size();
        // 继续查找下一个 delimiter 的位置
        end = str.find(delimiter, start);
        if (end == std::string::npos)
        {
            tokens.push_back(str.substr(start, str.size() - start));
        }
    }
    return tokens;
}



// 向 epollfd 添加文件描述符，并指定监听事件。edgeTrigger：边缘触发，isOneshot：EPOLLONESHOT
int addWaitFd(int epollFd, int newFd, bool edgeTrigger, bool isOneshot)
{
    epoll_event event;
    event.data.fd = newFd;
    // 监控读事件
    event.events = EPOLLIN;
    // 边缘触发模式
    if (edgeTrigger)
    {
        event.events |= EPOLLET;
    }
    // 单次触发模式
    if (isOneshot)
    {
        event.events |= EPOLLONESHOT;
    }
    int ret = epoll_ctl(epollFd, EPOLL_CTL_ADD, newFd, &event);
    if (ret != 0)
    {
        SPDLOG_ERROR("添加文件描述符失败");
        return -1;
    }
    return 0;
}

// 修改正在监听文件描述符的事件。edgeTrigger:是否为边沿触发，resetOneshot:是否设置 EPOLLONESHOT，addEpollout:是否监听输出事件
int modifyWaitFd(int epollFd, int modFd, bool edgeTrigger, bool resetOneshot, bool addEpollout)
{
    epoll_event event;
    event.data.fd = modFd;
    event.events = EPOLLIN;
    // 是否设置边缘触发模式
    if (edgeTrigger)
    {
        event.events |= EPOLLET;
    }
    // 是否设置一次性模式
    if (resetOneshot)
    {
        event.events |= EPOLLONESHOT;
    }
    // 添加写事件
    if (addEpollout)
    {
        event.events |= EPOLLOUT;
    }
    int ret = epoll_ctl(epollFd, EPOLL_CTL_MOD, modFd, &event);
    if (ret != 0)
    {
        SPDLOG_ERROR("修改文件描述符失败");
        return -1;
    }
    return 0;
}

// 删除正在监听的文件描述符
int deleteWaitFd(int epollFd, int deleteFd)
{
    int ret = epoll_ctl(epollFd, EPOLL_CTL_DEL, deleteFd, nullptr);
    if (ret != 0)
    {
        SPDLOG_ERROR("删除监听的文件描述符失败");
        return -1;
    }
    return 0;
}

// 设置文件描述符为非阻塞

int setNonBlocking(int fd)
{
    int oldFlag = fcntl(fd, F_GETFL);
    int ret = fcntl(fd, F_GETFL, oldFlag | O_NONBLOCK); // F_SETFL
    // int ret = fcntl(fd, F_SETFL, oldFlag); // F_SETFL
    if (ret != 0)
    {
        return -1;
    }
    return 0;
}
bool createDirectories(const std::string &path)
{
    std::mutex _lock;
    _lock.lock();
    // 检查路径是否存在
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
    {
        // 路径不存在，开始创建目录
        size_t pos = 0;
        std::string current_level = "";
        // 循环遍历路径中的每个目录级别
        while ((pos = path.find('/', pos + 1)) != std::string::npos)
        {
            current_level = path.substr(0, pos);
            if (stat(current_level.c_str(), &info) != 0)
            {
                // 如果当前级别目录不存在，则创建它
                if (mkdir(current_level.c_str(), 0755) != 0)
                {
                    // 创建目录失败
                    _lock.unlock();
                    return false;
                }
            }
        }
        // 检查最后一级目录是否创建
        if (stat(path.c_str(), &info) != 0)
        {
            if (mkdir(path.c_str(), 0755) != 0)
            {
                _lock.unlock();
                return false;
            }
        }
    }
    _lock.unlock();
    return true;
}
bool is_directory(const char *path)
{
    struct stat info;
    if (stat(path, &info) != 0)
    {
        SPDLOG_ERROR("访问路径时报错:{}",path);
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}
bool is_exits_path(const std::string &path)
{
    if (path.empty())
        return false;
    if (is_directory(path.c_str()))
    {
        struct stat info;
        if (stat(path.c_str(), &info) != 0)
        {
            return false;
        }
        return (info.st_mode & S_IFDIR) != 0;
    }
    else
    {
        struct stat buffer;
        return (stat(path.c_str(), &buffer) == 0);
    }
}
void initTopicFile(std::string path)
{
    std::ofstream urlInfoFile(path);
    // 检查文件是否成功打开
    if (!urlInfoFile.is_open())
    {
        std::cerr << "无法打开文件：" << path << std::endl;
        return;
    }
    urlInfoFile << "#HTTP请求的URL" << std::endl;
    urlInfoFile.close();
}
bool initServerFile(std::shared_ptr<CustomConfig> customConfig)
{
    char buffer[100];
    char *result = getcwd(buffer, 100);
    std::string path = result;
    path += "/ini";
    std::string topic_file_path = path + "/urlInfo.ini";
    std::string server_ini_path = path + "/serverInfo.ini";
    if (!is_exits_path(server_ini_path) || !is_exits_path(topic_file_path))
    {
        if (!is_exits_path(path))
        {
            bool is_ok = createDirectories(path);
            if (!is_ok)
                return false;
        }
        std::ofstream serverInfoFile(server_ini_path);
        // 检查文件是否成功打开
        if (!serverInfoFile.is_open())
        {
            std::cerr << "无法打开文件：" << server_ini_path << std::endl;
            return false;
        }
        serverInfoFile << "robot_user_name=root" << std::endl;
        serverInfoFile << "robot_user_ip=localhost" << std::endl;
        serverInfoFile << "robot_user_password=1" << std::endl;
        serverInfoFile << "robot_db_name=jsrobot" << std::endl;
        serverInfoFile << "port=8888" << std::endl;
        serverInfoFile << "# 0读取mysql数据 1读取本地数据" << std::endl;
        serverInfoFile << "read_status=1" << std::endl;
        serverInfoFile << "map_path=/opt/hura/dataset/map" << std::endl;
        serverInfoFile << "version_path=/opt/hura/version" << std::endl;
        serverInfoFile.close();

        initTopicFile(topic_file_path);
    }
    return true;
}
