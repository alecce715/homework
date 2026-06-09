# tinyhttpd 项目学习教程

## 一、项目概述

tinyhttpd 是一个轻量级的 HTTP 服务器实现，由 J. David Blackstone 编写，源代码仅约 500 行，是学习 HTTP 协议和服务器编程的绝佳范例。

### 项目特点

- 纯 C 语言实现，代码简洁易懂
- 支持 HTTP GET/POST 方法
- 支持静态文件服务
- 支持 CGI 程序执行
- 基于 POSIX 线程实现并发处理
- 跨平台兼容（Linux/Unix）

---

## 二、环境搭建与编译运行

### 2.1 环境要求

- Linux/Unix 系统（支持 POSIX 线程）
- GCC 编译器
- Make 构建工具

### 2.2 项目结构

```
tinyhttpd/
├── httpd.c          # 主程序源代码
├── Makefile         # 编译脚本
└── htdocs/          # 静态文件目录
    ├── index.html   # 默认首页
    ├── test.html    # 测试页面
    └── cgi-bin/     # CGI 脚本目录
        └── hello    # 示例 CGI 脚本
```

### 2.3 编译步骤

```bash
# 进入项目目录
cd tinyhttpd

# 使用 Makefile 编译
make

# 清理编译产物
make clean
```

### 2.4 运行服务器

```bash
# 默认端口 8080 启动
./httpd

# 输出：httpd running on port 8080
```

### 2.5 测试访问

```bash
# 访问首页
curl http://localhost:8080/

# 访问测试页面
curl http://localhost:8080/test.html

# 访问 CGI 脚本
curl http://localhost:8080/cgi-bin/hello
```

---

## 三、核心模块功能说明

### 3.1 主函数（main）

主函数负责服务器的初始化和主循环：

```c
int main(void) {
    int server_sock = -1;
    u_short port = DEFAULT_PORT;
    int client_sock = -1;
    pthread_t newthread;

    server_sock = startup(&port);           // 创建监听套接字
    printf("httpd running on port %d\n", port);

    while (1) {
        client_sock = accept(server_sock, ...);  // 接受客户端连接
        pthread_create(&newthread, NULL, accept_request, &client_sock);  // 创建线程处理
    }
    return 0;
}
```

**功能流程：**
1. 调用 `startup()` 创建并绑定监听套接字
2. 进入无限循环，通过 `accept()` 等待客户端连接
3. 每接收到一个连接，创建新线程处理请求
4. 主线程继续等待下一个连接

### 3.2 服务器启动（startup）

```c
int startup(u_short *port) {
    int httpd = socket(PF_INET, SOCK_STREAM, 0);  // 创建 TCP 套接字
    
    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_port = htons(*port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);     // 绑定所有网络接口
    
    bind(httpd, (struct sockaddr *)&name, sizeof(name));  // 绑定端口
    listen(httpd, 5);  // 开始监听，backlog=5
    
    return httpd;
}
```

**关键点：**
- `INADDR_ANY` 表示监听所有可用的网络接口
- `listen()` 的第二个参数是等待队列长度
- 如果端口设为 0，系统会动态分配一个可用端口

### 3.3 请求处理（accept_request）

这是核心的请求处理函数，运行在独立线程中：

```c
void accept_request(void *arg) {
    int client = *(int *)arg;
    char buf[1024];
    char method[255], url[255], path[512];
    int cgi = 0;
    char *query_string = NULL;

    // 1. 读取请求行
    numchars = get_line(client, buf, sizeof(buf));
    
    // 2. 解析 HTTP 方法
    while (!isspace(buf[j]) && (i < sizeof(method) - 1)) {
        method[i] = buf[j];
        i++; j++;
    }
    method[i] = '\0';

    // 3. 检查是否支持的方法（仅支持 GET/POST）
    if (strcasecmp(method, "GET") && strcasecmp(method, "POST")) {
        unimplemented(client);
        return;
    }

    // 4. 解析 URL 和查询字符串
    // ... URL 解析逻辑 ...

    // 5. 判断是否为 CGI 请求
    if (strcasecmp(method, "POST") == 0) cgi = 1;
    // 或 URL 包含 '?' 或文件可执行

    // 6. 处理请求
    if (!cgi)
        serve_file(client, path);      // 静态文件
    else
        execute_cgi(client, path, method, query_string);  // CGI 执行
}
```

**请求处理流程：**
1. 读取 HTTP 请求行
2. 解析请求方法（GET/POST）
3. 解析请求 URL 和查询字符串
4. 判断请求类型（静态文件 vs CGI）
5. 调用相应的处理函数

### 3.4 静态文件服务（serve_file）

```c
void serve_file(int client, const char *filename) {
    FILE *resource = fopen(filename, "r");
    
    if (resource == NULL) {
        not_found(client);  // 文件不存在
    } else {
        headers(client);    // 发送响应头
        cat(client, resource);  // 发送文件内容
    }
    
    fclose(resource);
}
```

**响应头结构：**
```
HTTP/1.0 200 OK
Server: tinyhttpd/0.1.0
Content-Type: text/html

[文件内容]
```

### 3.5 CGI 执行（execute_cgi）

CGI（Common Gateway Interface）允许服务器执行外部程序并返回结果：

```c
void execute_cgi(int client, const char *path, const char *method, const char *query_string) {
    int cgi_output[2], cgi_input[2];
    pid_t pid;

    pipe(cgi_output);  // 创建输出管道
    pipe(cgi_input);   // 创建输入管道

    if ((pid = fork()) == 0) {
        // 子进程：执行 CGI 程序
        dup2(cgi_output[1], 1);  // 重定向 stdout
        dup2(cgi_input[0], 0);   // 重定向 stdin
        
        // 设置环境变量
        putenv("REQUEST_METHOD=GET");
        putenv("QUERY_STRING=...");
        
        execl(path, path, NULL);  // 执行 CGI 程序
    } else {
        // 父进程：读取 CGI 输出并发送给客户端
        close(cgi_output[1]);
        while (read(cgi_output[0], &c, 1) > 0)
            send(client, &c, 1, 0);
        
        waitpid(pid, &status, 0);  // 等待子进程结束
    }
}
```

**CGI 执行流程：**
1. 创建管道用于进程间通信
2. fork 创建子进程
3. 子进程重定向标准输入输出
4. 设置环境变量（REQUEST_METHOD, QUERY_STRING 等）
5. 执行 CGI 程序
6. 父进程读取输出并发送给客户端

### 3.6 多线程处理

tinyhttpd 使用 POSIX 线程实现并发：

```c
pthread_t newthread;
pthread_create(&newthread, NULL, (void *)accept_request, (void *)&client_sock);
```

**特点：**
- 每个连接由独立线程处理
- 主线程专注于接受新连接
- 线程资源由系统自动回收（线程退出后）

---

## 四、关键函数调用关系与数据流

### 4.1 函数调用关系图

```
main()
    │
    ├─► startup()           # 创建监听套接字
    │
    └─► accept()            # 等待客户端连接
            │
            └─► pthread_create()
                    │
                    └─► accept_request()  # 处理请求
                            │
                            ├─► get_line()           # 读取请求行
                            ├─► serve_file()         # 静态文件服务
                            │       ├─► headers()
                            │       └─► cat()
                            ├─► execute_cgi()        # CGI 执行
                            │       ├─► pipe()
                            │       ├─► fork()
                            │       └─► execl()
                            ├─► not_found()          # 404 错误
                            └─► unimplemented()      # 501 错误
```

### 4.2 数据流分析

**请求数据流：**
```
客户端 ──► TCP Socket ──► accept() ──► client_sock
                                           │
                                           ▼
                                   accept_request()
                                           │
                        ┌─────────────────┼─────────────────┐
                        ▼                 ▼                 ▼
                   get_line()        解析方法         解析URL
                        │                 │                 │
                        └─────────────────┼─────────────────┘
                                          ▼
                                    判断请求类型
                                          │
                        ┌─────────────────┴─────────────────┐
                        ▼                                   ▼
                   serve_file()                      execute_cgi()
                        │                                   │
                        ▼                                   ▼
                   读取文件内容                      fork+exec CGI
                        │                                   │
                        └─────────────────┬─────────────────┘
                                          ▼
                                    send() 发送响应
                                          │
                                          ▼
                                   close(client)
```

**HTTP 请求解析流程：**
```
原始请求：GET /index.html?name=test HTTP/1.0\r\n...

解析步骤：
1. 提取方法：GET
2. 提取 URL：/index.html?name=test
3. 分离路径和查询字符串：
   - 路径：/index.html
   - 查询字符串：name=test
4. 构建本地路径：htdocs/index.html
5. 判断是否可执行（CGI）
```

---

## 五、架构设计与性能改进思考题

### 5.1 问题一：线程模型的改进

**问题：** 当前实现中，每接收到一个连接就创建一个新线程。在高并发场景下，这种模型有什么问题？

**思考方向：**
- 线程创建和销毁的开销
- 内存占用（每个线程有独立的栈空间）
- 线程数量无限制可能导致资源耗尽

**改进方案：**
- 线程池模式：预先创建固定数量的线程
- 使用事件驱动模型（如 epoll）
- 考虑协程替代线程

### 5.2 问题二：请求解析的健壮性

**问题：** 当前的请求解析存在哪些安全隐患？

**思考方向：**
- 缓冲区溢出风险（`buf[1024]` 固定大小）
- 路径遍历攻击（`../../etc/passwd`）
- 超长 URL 攻击
- HTTP 请求走私

**改进方案：**
- 增加输入验证和边界检查
- 使用动态内存分配
- 实现 URL 规范化和路径安全检查

### 5.3 问题三：性能优化策略

**问题：** 如何提升 tinyhttpd 的吞吐量和响应速度？

**思考方向：**
- 文件读取效率（当前使用 `fgets` 逐行读取）
- 网络发送效率（当前使用 `send` 单字节发送）
- 缓存机制
- 并发模型优化

**改进方案：**
- 使用 `mmap` 映射文件到内存
- 使用 `writev` 批量发送数据
- 实现静态文件缓存
- 引入事件驱动（如 libevent）

### 5.4 问题四：功能扩展

**问题：** 如果要支持 HTTPS，需要做哪些修改？

**思考方向：**
- TLS 协议实现
- 证书管理
- 加密性能开销

**改进方案：**
- 集成 OpenSSL 库
- 实现 SSL/TLS 握手
- 考虑使用现成的 TLS 库封装

---

## 六、总结

tinyhttpd 是一个优秀的 HTTP 服务器学习范例，通过约 500 行代码展示了：

1. **网络编程基础**：套接字创建、绑定、监听、接受连接
2. **HTTP 协议解析**：请求行、请求头、URL 解析
3. **并发处理**：POSIX 线程模型
4. **静态文件服务**：文件读取和响应构建
5. **CGI 机制**：进程创建、管道通信、环境变量传递

学习 tinyhttpd 能帮助理解 Web 服务器的核心工作原理，是深入学习网络编程的良好起点。