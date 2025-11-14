cmake_minimum_required(VERSION 3.0.2)
project(customServer)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

# # 设置 MySQL 头文件路径和库文件路径
# set(MYSQL_INCLUDE_DIRS "/usr/include/mysql")
# set(MYSQL_LIBRARIES "-L/usr/lib/x86_64-linux-gnu/mysql -lmysqlclient")
# # 设置 MQTT 头文件路径和库文件路径
# set(MQTT_INCLUDE_DIRS "/home/chenlang/SDK/mqtt/common/src/mosquitto-1.6.9/install/include")
# set(MQTT_LIBRARIES "-L/home/chenlang/SDK/mqtt/common/src/mosquitto-1.6.9/install/lib -lmosquitto")

# set(YAML_CPP_INCLUDE_DIRS "/usr/local/include")
# set(YAML_CPP_LIBRARIES "-L/usr/local/lib -lyaml-cpp")

#查找系统安装的库
# find_package( REQUIRED)

set(ALL_TARGET_LIBRARIES "")
# list(APPEND ALL_TARGET_LIBRARIES  -lpthread ${YAML_CPP_LIBRARIES}  ${MYSQL_LIBRARIES} ${MQTT_LIBRARIES}  )
list(APPEND ALL_TARGET_LIBRARIES  -lpthread )

include_directories(
  include
)

# 将src目录下的所有源文件添加到变量SRC中
aux_source_directory(src SRC)
# 生成可执行文件
add_executable(customServer ${SRC})
# 指定可执行文件的依赖项
target_link_libraries(customServer ${ALL_TARGET_LIBRARIES} ) 











