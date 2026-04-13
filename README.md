# BrokenWebServer

一个从零实现、按版本持续演进的 C++ 小型 Web 服务器项目。

这个仓库不是“把现成框架拼起来”的演示代码，而是一个带有明确学习路径的服务端练手项目：从最早的单机版 HTTP 原型，一步步演进到基于 `Reactor + epoll + 线程池 + MySQL` 的多连接服务器。当前最完整、最推荐阅读和运行的版本是 `v4`。

项目适合用来学习这些主题：

- Linux 网络编程
- `epoll` 事件驱动模型
- Reactor 思想与主从职责拆分
- HTTP 请求解析与响应构造
- 线程池与任务队列
- MySQL 登录/注册流程的最小实现

## 项目特点

- 从 `v1` 到 `v4` 逐步演进，便于按阶段理解设计变化
- 基于 `epoll` 的事件循环，支持多连接处理
- 主线程负责 IO 复用与事件分发，逻辑线程池负责业务处理
- 支持静态页面返回与自定义错误页
- 支持简单的登录 / 注册表单处理
- 支持接入 MySQL 做用户存储
- 支持守护进程方式启动：`-d` / `--daemon`
- 保留了较多真实开发痕迹，适合边读边改边复盘

## 目录结构

```text
brokenWebserver/
├── README.md
├── 性能测试.md
├── epollpra/                 # epoll 相关练习代码
├── v1/                       # 最初版：最基础的 HTTP 访问能力
├── v2/                       # 引入多连接与线程池思路
├── v3/                       # 引入更清晰的 IO / Logic 分层
└── v4/                       # 当前主版本，推荐阅读与运行
    ├── broken_include/       # 公共头文件
    ├── broken_src/           # 入口、Makefile、静态页面、SQL、脚本
    ├── http/                 # HTTP 解析、响应构造、请求处理
    ├── io/                   # Socket、Connection、epoll 封装
    ├── logic/                # 任务队列、线程池、页面与 MySQL 逻辑
    └── server/               # 服务主循环、信号与定时器事件
```

## 版本演进

### `v1`

第一版目标很直接：先把一个最小可访问的 HTTP 服务器跑起来。

- 能监听端口并返回基础页面
- 完成最基本的请求解析与响应返回
- 整体还是偏单线程、原型验证风格

### `v2`

开始从“能跑”走向“能同时处理更多连接”。

- 引入多连接处理思路
- 增加线程池，尝试把任务分发出去
- 开始对 socket、线程同步等基础能力做封装

### `v3`

进入比较明确的分层阶段。

- 区分 HTTP 层、IO 层、逻辑层
- 使用 `epoll` 做事件分发
- 增加错误页和更完整的请求处理流程
- 已经具备一个小型 Web Server 的基本骨架

### `v4`

当前主版本，在 `v3` 基础上继续补全工程化能力。

- 保留 `epoll + Reactor` 主循环
- 使用逻辑线程池处理业务任务
- 接入 MySQL，支持注册 / 登录
- 增加表单解析、状态页、数据库初始化脚本
- 支持守护进程启动
- 更适合作为当前阅读、展示和后续继续扩展的版本

## `v4` 架构说明

### 整体职责划分

- `server/`：维护主事件循环，处理监听 socket、连接收发、`signalfd`、`timerfd`
- `io/`：管理连接对象、非阻塞读写、`epoll` 注册与连接生命周期
- `http/`：负责请求解析、响应头生成、静态文件响应拼装
- `logic/`：负责业务任务队列、线程池、页面路由、MySQL 登录注册逻辑
- `broken_src/`：提供启动入口、静态资源、错误页、SQL 和构建入口

### 当前处理流程

`accept -> epoll 收到可读事件 -> 读取请求 -> HTTP 解析 -> 投递到逻辑线程池 -> 页面/登录逻辑处理 -> 构造响应 -> 主线程回写 -> 关闭连接`

### 目前已实现的能力

- 处理 `GET` / `POST` 请求
- 返回静态页面
- 返回 `403`、`404`、`500`、`504` 错误页
- `/login`、`/register`、`/auth` 路由复用登录页面与提交逻辑
- 解析 `application/x-www-form-urlencoded` 表单
- MySQL 用户注册与登录校验
- 支持非阻塞写回与定时重试
- 支持 `SIGINT` / `SIGTERM` 优雅停止

## 运行环境

这个项目明显偏 Linux 环境，`v4` 依赖这些系统能力：

- `epoll`
- `signalfd`
- `timerfd`
- POSIX socket
- MySQL C API

建议环境：

- Ubuntu 22.04
- `g++`
- `make`
- `libmysqlclient-dev`
- 可选：本地 MySQL Server

## 编译与运行

推荐在 [`v4/broken_src`](./v4/broken_src) 目录下操作，因为当前静态资源路径依赖工作目录。

### 1. 编译

```bash
cd v4/broken_src
make
```

如果本机 MySQL 头文件或库路径不同，可以在编译时覆盖变量：

```bash
make MYSQL_INCLUDE=/usr/include/mysql MYSQL_LIBS='-lmysqlclient'
```

### 2. 前台运行

```bash
./server
```

默认监听地址：

```text
http://127.0.0.1:8080/
```

### 3. 守护进程运行

```bash
./server --daemon
```

或：

```bash
./server -d
```

## MySQL 配置

`v4` 中的登录/注册逻辑支持通过环境变量配置数据库连接。

可用环境变量：

- `BROKEN_SERVER_DB_HOST`
- `BROKEN_SERVER_DB_PORT`
- `BROKEN_SERVER_DB_NAME`
- `BROKEN_SERVER_DB_USER`
- `BROKEN_SERVER_DB_PASSWORD`
- `BROKEN_SERVER_DB_TABLE`
- 兼容回退：`MYSQL_PWD`

仓库里已经提供示例脚本：[`v4/broken_src/deploy_mysql.example.sh`](./v4/broken_src/deploy_mysql.example.sh)



### 初始化数据库

项目提供了初始化脚本：[`v4/broken_src/init_login_schema.sql`](./v4/broken_src/init_login_schema.sql)

执行示例：

```bash
mysql -h 127.0.0.1 -P 3306 -u root -p < init_login_schema.sql
```

脚本会：

- 创建数据库 `broken_server`
- 创建用户表 `users`
- 插入演示账号 `demo / 123456`

另外，服务启动时也会尝试自动创建数据库和表；如果 MySQL 无法连接，服务仍可启动，但会打印告警，此时登录/注册功能不可用。

## 可直接测试的页面与路由

在 `v4/broken_src` 下已经放好了静态资源与错误页，可直接验证：

- `/`：首页
- `/index.html`：首页静态文件
- `/login`：登录页入口
- `/register`：注册页入口
- `/auth`：表单提交处理入口
- `/403.html`：403 页面预览
- `/404.html`：404 页面预览
- `/500.html`：500 页面预览
- `/504.html`：504 页面预览
- `/missing-page`：触发真实 404

## 性能测试记录

仓库根目录有一份单独的压测记录：[`性能测试.md`](./性能测试.md)

当前记录环境：

- Ubuntu 22.04
- 4 核 CPU
- 4 GB 内存
- 压测工具：`webbench`

文档中记录了几组测试：

- `500 clients / 60s`：全部成功返回
- `5000 clients / 60s`：仍可全部成功返回，但吞吐没有继续线性上升
- `50000 clients / 60s`：压测端 `fork` 资源不足，测试本身先失败

从现有结果看，这个项目更适合作为架构学习和工程拆分练习，而不是高性能生产级 Web Server。性能瓶颈主要还在当前 IO 与整体实现方式上，后续仍有较大优化空间。

## 这个项目适合怎么读

如果你是第一次看这个仓库，建议顺序：

1. 先看 `v1`，理解最小 HTTP 服务器原型
2. 再看 `v2` / `v3`，观察多连接、线程池、分层的引入过程
3. 最后重点读 `v4`，把主循环、HTTP、逻辑层、数据库接起来看

如果你只是想直接跑通一个当前版本，建议直接从 `v4/broken_src` 开始。

## 说明

这个项目最有价值的地方，不只是“最后跑起来的结果”，而是它完整保留了一个 Web 服务器从原型到逐步分层、逐步补功能的演进过程。

如果你正在学习 C++ 服务端、`epoll`、线程池、HTTP 处理，或者想找一个适合自己继续改造的小型项目，这个仓库会是一个很好的练手材料。
