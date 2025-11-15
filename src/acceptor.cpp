#include "acceptor.h"

Acceptor::Acceptor(int port,const char *ip):listen_port(port)
{
    // 指定地址
    bzero(&m_serverAddr, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(port);
    // 根据传入的 ip 确定是否指定 ip 地址
    if (ip != nullptr)
    {
        m_serverAddr.sin_addr.s_addr = inet_addr(ip);
    }
    else
    {
        m_serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    SPDLOG_INFO("port:{}",port);
    // 创建套接字
    listen_fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_fd < 0)
    {
        SPDLOG_ERROR("套接字创建失败,error:{}",std::strerror(errno));
        return;
    }
    // 设置地址可重用
    int reuseAddr = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));
    if (ret != 0)
    {
        SPDLOG_ERROR("套接字设置地址重用失败,error:{}",std::strerror(errno));
        return;
    }

    // 绑定地址信息
    ret = bind(listen_fd, (sockaddr *)&m_serverAddr, sizeof(m_serverAddr));
    if (ret != 0)
    {
        SPDLOG_ERROR("套接字绑定地址失败,error:{}",std::strerror(errno));
        return;
    }

    // 开启监听
    ret = listen(listen_fd, MaxListenFd);
    if (ret != 0)
    {
        SPDLOG_ERROR("套接字开启监听失败,error:{}",std::strerror(errno));
        return;
    }
}
int Acceptor::acceptClient()
{
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connect_fd = accept(listen_fd, (sockaddr *)&client_addr, &client_len); 
    // SPDLOG_TRACE("在监听套接字上进行连接");
    return connect_fd;
}
