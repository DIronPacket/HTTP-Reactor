cmake_minimum_required(VERSION 3.0.2)
project(customServer)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

# 设置 SPDLOG 头文件路径和库文件路径
set(SPDLOG_INCLUDE_DIRS "/home/djy/workspace/HTTP-Reactor/install/include")
set(SPDLOG_LIBRARIES "-L/home/djy/workspace/HTTP-Reactor/install/lib -lspdlogd")

set(ALL_TARGET_LIBRARIES "")
# list(APPEND ALL_TARGET_LIBRARIES  -lpthread ${YAML_CPP_LIBRARIES}  ${MYSQL_LIBRARIES} ${MQTT_LIBRARIES}  )
list(APPEND ALL_TARGET_LIBRARIES  -lpthread ${SPDLOG_LIBRARIES})

include_directories(
  include
  ${SPDLOG_INCLUDE_DIRS}
)

# 将src目录下的所有源文件添加到变量SRC中
aux_source_directory(src SRC)
# 生成可执行文件
add_executable(customServer ${SRC})
# 指定可执行文件的依赖项
target_link_libraries(customServer ${ALL_TARGET_LIBRARIES} ) 
# 精确代码位置日志 - 开发环境下开启（有性能开销）
# target_compile_definitions(customServer PRIVATE SPDLOG_FMT_EXTERNAL)
target_compile_definitions(customServer PRIVATE
    # 这将完美解决 "fmt/core.h not found" 的问题
    SPDLOG_FMT_HEADER_ONLY
    # 打开 TRACE 级别的源码定位
    SPDLOG_TRACE_ON
    # 打开 DEBUG 级别的源码定位
    SPDLOG_DEBUG_ON
    #需要开启并设置，否则默认INFO级别
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE 
    # 这个宏用于显示文件名和行号，可以保留
    # SPDLOG_FMT_EXTERNAL
)











