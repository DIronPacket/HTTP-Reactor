#include "myserver.h"
#include "customConfig.h"

//@@ 
//@spdlog消息级别：trace debug info warn err critical(致命)
//@@
// 只需要在程序启动时配置一次
void initialize_logging() {

    // --- 【异步设置核心部分】 ---
    // 1. 创建 spdlog 的全局线程池
    //    参数1: 队列大小 (必须是2的幂)
    //    参数2: 后台处理线程的数量
    spdlog::init_thread_pool(8192, 1); // 队列大小8192，1个后台线程


    // 2. 创建两个不同的 Sink，【关键】为每个 Sink 设置独立的级别
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    // 控制台只显示 INFO 及以上级别的信息，保持清爽
    console_sink->set_level(spdlog::level::info); 
    console_sink->set_pattern("%^[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v %$");

    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("logs/detailed.log", false);
       // 文件里记录所有 TRACE 及以上级别的日志，用于详细排查
    file_sink->set_level(spdlog::level::trace);
    file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");

    // // 3. 将 Sink 组合起来
    std::vector<spdlog::sink_ptr> sinks {console_sink, file_sink};
    // auto logger = std::make_shared<spdlog::logger>("multi_level_logger", sinks.begin(), sinks.end());
    // 3.异步组合
    auto logger = std::make_shared<spdlog::async_logger>("async_logger", sinks.begin(), sinks.end(),
                spdlog::thread_pool(),
                spdlog::async_overflow_policy::block//队列满时阻塞
            ); 

    // 4. 【关键】设置 Logger 的总级别为最低，确保它不会提前拦截任何消息
    logger->set_level(spdlog::level::trace);

    // 5. 注册并设为默认
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);

    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
    SPDLOG_TRACE("trace inti successfully");
    SPDLOG_DEBUG("debug init successfully");
    SPDLOG_INFO("info init successfully");
    // SPDLOG_ERROR("error init successfully");
    // SPDLOG_INFO("async logger async? {}" , dynamic_cast<spdlog::async_logger*>(logger.get()) != nullptr);
}

int main(int argc, char *argv[])
{
    // 配置服务端
    std::shared_ptr<CustomConfig> customConfig = std::make_shared<CustomConfig>();
    bool is_config = customConfig.get()->initConfig();

    //初始化spdlog
    // spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%@ %!] %v");
    // spdlog::set_level(spdlog::level::trace);
    // spdlog::trace("start server");
    // SPDLOG_DEBUG("这是一条来自 my_module 的 debug 日志。");
    initialize_logging();
    SPDLOG_TRACE("服务器初始化配置数据完毕！");
    if (!is_config)
    {
        initServerFile(customConfig);
        is_config = customConfig.get()->initConfig();
        if (!is_config)
        {
            SPDLOG_ERROR("服务初始化失败，请尝试删除 ini 文件夹并重启服务！");
        }
    }
    // 根据端口号和线程数构造http_server
    MyServer http_server(customConfig.get()->serverInfo.port, 8, customConfig);
    http_server.start();
    spdlog::shutdown();
    return 0;
}