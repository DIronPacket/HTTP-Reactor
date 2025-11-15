# 基于单一Reactor模式的优化版本：主从Reacotr模式
## 一.组成模块
&emsp; &emsp;**主Reactor**：负责监听所有HTTP连接请求，监听到请求后创建连接，并间接交给子Reactor线程池。
<br/>&emsp; &emsp;**子Reactor线程池**：拿到主Reactor的连接后，解析读事件，提交请求到业务线程池，获取响应后封装写事件。
<br/>&emsp; &emsp;**业务线程池**：处理子Reactor线程池提交的任务，进行业务处理。


# 优化情况
* ./bench/20251115

# 下一步持续优化

# 目标
