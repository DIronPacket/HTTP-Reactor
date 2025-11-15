#include "customConfig.h"

CustomConfig::CustomConfig()
{
    char buffer[100];
    char *result = getcwd(buffer, 100);
    urlPath = result;
    initPath += urlPath + "/ini/serverInfo.ini";
    urlPath += "/ini/urlInfo.ini";
}

bool CustomConfig::initConfig()
{
    // 判断传入的路径是否为空或者文件是否存在
    if (initPath.empty() || !std::ifstream(initPath).good() || urlPath.empty() || !std::ifstream(urlPath).good())
        return false;
    // 打开配置文件
    std::ifstream iniServerFile(initPath), iniTopicFile(urlPath);
    if (!iniServerFile.is_open())
    {
        SPDLOG_ERROR("无法打开服务端配置文件：{}",initPath);
        return false;
    }
    std::string lineStr, configKey, configValue;
    // 读取服务端配置数据
    while (std::getline(iniServerFile, lineStr))
    {
        if (lineStr.size() > 0 && lineStr[0] != '#')
        {
            int index_ = lineStr.find("=");
            configKey = lineStr.substr(0, index_);
            configValue = lineStr.substr(index_ + 1, lineStr.size() - index_ - 1);
            iniDataMap[configKey] = configValue;
            // std::cout << "key:" << configKey << "   value:" << configValue << std::endl;
        }
    }
    iniServerFile.close();

    if (!iniTopicFile.is_open())
    {
        SPDLOG_ERROR("无法打开服务端配置文件：{}",urlPath);
        return false;
    }
    // 读取HTTP通信的topic数据
    while (std::getline(iniTopicFile, lineStr))
    {
        if (lineStr.size() > 0 && lineStr[0] != '#')
        {
            int index_ = lineStr.find("=");
            configKey = lineStr.substr(0, index_);
            configValue = lineStr.substr(index_ + 1, lineStr.size() - index_ - 1);
            iniDataMap[configKey] = configValue;
            // std::cout << "key:" << configKey << "   value:" << configValue << std::endl;
        }
    }
    iniTopicFile.close();
    return getConfigData();
}
bool CustomConfig::getConfigData()
{
    try
    {
        // 获取server的配置信息
        serverInfo.port = std::stoi(iniDataMap.at("port"));
        serverInfo.read_status = std::stoi(iniDataMap.at("read_status"));
        serverInfo.map_path = iniDataMap.at("map_path");
        serverInfo.version_path = iniDataMap.at("version_path");
        //获取mysql的配置信息
        mysqlInfo.robot_user_name=iniDataMap.at("robot_user_name");
        mysqlInfo.robot_user_ip=iniDataMap.at("robot_user_ip");
        mysqlInfo.robot_user_password=iniDataMap.at("robot_user_password");
        mysqlInfo.robot_db_name=iniDataMap.at("robot_db_name");
        //获取服务器指定的url信息
    }
    catch (const std::exception &e)
    {
        SPDLOG_ERROR("获取.ini的配置信息时出现错误:{}",e.what());
        return false;
    }
    return true;
}
std::string CustomConfig::readConfig(const std::string &key)
{
}
void CustomConfig::writeConfig(const std::string &key, const std::string &value)
{
}