#ifndef UTILS_H
#define UTILS_H
#pragma once
#include <iostream>
#include <ctime>
#include <chrono>
#include <vector>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/epoll.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <memory>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <mutex>
#include "read_write_lock.h"
#include "customConfig.h"
// 包含 spdlog 的核心头文件
#include "spdlog/spdlog.h"
#include "spdlog/async.h"
// 包含你需要的 sink 头文件
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include <cstring> // For std::strerror
#include <cerrno>  // For errno
#include <mutex>
class CustomConfig;
// 以 "09:50:19.0619 2022-09-26 [logType]: " 格式返回当前的时间和输出类型，logType 指定输出的类型：
// init  : 表示服务器的初始化过程
// error : 表示服务器运行中的出错消息
// info  : 表示程序的运行信息
enum THREADTYPE
{
    MAIN_REACTOR, // 主线程
    SUB_REACTOR,  // 子线程
    WORK_THREAD,   // 工作线程
    ERROR_LOG   //报错日志
};

// 向 epollfd 添加文件描述符，并指定监听事件。edgeTrigger：边缘触发，isOneshot：EPOLLONESHOT
int addWaitFd(int epollFd, int newFd, bool edgeTrigger = false, bool isOneshot = false);

// 修改正在监听文件描述符的事件。edgeTrigger:是否为边沿触发，resetOneshot:是否设置 EPOLLONESHOT，addEpollout:是否监听输出事件
int modifyWaitFd(int epollFd, int modFd, bool edgeTrigger = false, bool resetOneshot = false, bool addEpollout = false);

// 删除正在监听的文件描述符
int deleteWaitFd(int epollFd, int deleteFd);

// 设置文件描述符为非阻塞
int setNonBlocking(int fd);

// 获取当前日期作为日志文件名
void getDateToFileName(std::string &file_name);

std::vector<std::string> split(const std::string &str, const std::string &delimiter);

inline int getStrSize(std::string str_)
{
    return str_.size();
}

// 创建指定路径文件夹
bool createDirectories(const std::string &path);
// 判断指定路径是否是文件夹
bool is_directory(const char *path);
bool is_exits_path(const std::string &path);
//初始化服务端配置文件
bool initServerFile(std::shared_ptr<CustomConfig> customConfig);
void initTopicFile(std::string path);
#endif