#ifndef CUSTOMCONFIG_H
#define CUSTOMCONFIG_H

#include <iostream>
#include <string>
#include <utils.h>
#include <unistd.h>
#include <map>
#include <fstream>
struct ServerUrl
{
    
};
struct ServerInfo
{
    int port;
    int read_status;
    std::string map_path;
    std::string version_path;
};
struct MysqlInfo
{
    std::string robot_user_name;
    std::string robot_user_ip;
    std::string robot_user_password;
    std::string robot_db_name;
}; 
class CustomConfig
{
public:
    CustomConfig();
    bool initConfig();

    std::string readConfig(const std::string &key);
    void writeConfig(const std::string &key, const std::string &value);

private:
    bool getConfigData();

public:
    ServerUrl urlInfo;
    ServerInfo serverInfo;
    MysqlInfo mysqlInfo;
    std::string initPath;
    std::string urlPath;
    std::map<std::string, std::string> iniDataMap;
};
#endif
