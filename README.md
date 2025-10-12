# 基于单一Reactor模式的优化版本：主从Reacotr模式
## 一.组成模块
&emsp; &emsp;**主Reactor**：负责监听所有HTTP连接请求，监听到请求后创建连接，并间接交给子Reactor线程池。
<br/>&emsp; &emsp;**子Reactor线程池**：拿到主Reactor的连接后，解析读事件，提交请求到业务线程池，获取响应后封装写事件。
<br/>&emsp; &emsp;**业务线程池**：处理子Reactor线程池提交的任务，进行业务处理。


# 后续设计演进
## 整体架构：
&emsp; &emsp;__主Reactor__：负责监听所有HTTP连接请求，监听到请求后创建连接，并间接交给子Reactor线程池。
<br/>&emsp; &emsp;__从Reactor线程池__：专注epoll_wait（主要EPOLLIN），轻量处理读事件（添加优先级，提交业务池）。循环中检查通知队列，执行fd关闭/重置。
<br/>&emsp; &emsp;__业务处理线程池__：解析请求，生成响应，创建WriteTask（含fd、data、priority、keep_alive），入优先级队列 + sem_post通知写池。
<br/>&emsp; &emsp;__写工作线程池（一个池，线程数=CPU核心）__：sem_wait等待，弹出任务，直接non-blocking send；EAGAIN重入队列；写完enqueue通知从Reactor关闭fd。
## 核心流程：<br/>
从Reactor检测EPOLLIN → 读请求 → 计算优先级 → 提交业务池。<br/>
业务池处理 → 生成响应 → 入优先级队列（std::priority_queue） + sem_post。<br/>
写池：sem_wait → 弹出（top/pop） → 分块send（MSG_DONTWAIT） → EAGAIN重入队列 + sem_post → 写完enqueue通知从Reactor。<br/>
从Reactor：检查通知队列 → epoll_ctl DEL + close(fd)（或MOD为EPOLLIN重置）。<br/>

## 关键组件：

<br/>&emsp; &emsp;优先级队列：全局std::priority_queue<WriteTask>，mutex保护；弹出按priority（高先出）。
<br/>&emsp; &emsp;信号量：sem_t控制写池唤醒，避免忙等。
<br/>&emsp; &emsp;通知队列：无锁ConcurrentQueue<CloseNotify>（fd + close_flag），写池到从Reactor单向通信。
<br/>&emsp; &emsp;WriteTask结构：{fd, data, priority, keep_alive}。