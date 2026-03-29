# v11

这是一个简单的 C++ HTTP 静态文件服务器练习版本，源码主要放在 `broken_src/`，头文件放在 `broken_include/`。

当前默认监听端口是 `8080`，启动后会尝试处理浏览器发来的 HTTP 请求，并返回当前目录下的静态文件；访问 `/` 时会默认返回 `index.html`。

## 目录结构

- `broken_include/`：头文件目录
- `broken_src/`：源文件、`makefile`、示例页面 `index.html`
- `broken_src/main.cpp`：程序入口
- `broken_src/httpserver.cpp`：请求处理和静态文件响应逻辑
- `broken_src/httpprase.cpp`：HTTP 请求解析逻辑
- `broken_src/httpreponse.cpp`：HTTP 响应相关逻辑
- `broken_src/Socket.cpp`：套接字封装
- `broken_src/tooh.cpp`：字符串辅助工具

## 功能说明

- 支持基础的 HTTP 请求行解析
- 支持读取部分常见请求头
- 支持返回静态文件
- 支持根据文件后缀设置基础 `Content-Type`
- 根路径 `/` 默认映射到 `index.html`

## 构建方式

项目里的 `makefile` 使用的是 `g++`，更偏向 Linux / MinGW 环境。

在 `broken_src/` 目录下可以使用类似命令：

```bash
g++ -std=c++17 -Wall -g main.cpp httpserver.cpp httpprase.cpp httpreponse.cpp Socket.cpp tooh.cpp -o server
```

如果环境里安装了 `make`，也可以直接：

```bash
make
```

## 运行方式

在 `broken_src/` 目录下运行：

```bash
./server
```

然后浏览器访问：

```text
http://127.0.0.1:8080/
```

## 当前默认页面

`broken_src/index.html` 的内容很简单，用来验证服务器是否成功返回静态页面：

- 页面标题：`My Server`
- 页面正文：`It works!`

## 注意事项

- 这份代码明显是练习/调试版本，目录名中的 `broken_*` 也说明它不是一个完全整理好的最终版本。
- 文件名里存在一些拼写问题，例如 `httpprase.cpp`、`httpreponse.cpp`、`tooh.cpp`，但当前构建脚本就是按这些名字引用的。
- 代码中使用了 `sys/socket.h`、`arpa/inet.h`、`unistd.h`、`sys/mman.h` 等接口，因此更适合在类 Unix 环境，或具备兼容层的 MinGW 环境下编译运行。
- 如果直接在普通 Windows PowerShell 环境中执行 `make`，可能会提示命令不存在，需要额外安装构建工具。

## 适合做什么

这个版本比较适合用来：

- 学习 HTTP 请求的基础解析流程
- 理解静态文件服务器的最小实现
- 练习 socket 编程
- 继续做重构、修 bug 或补充功能
