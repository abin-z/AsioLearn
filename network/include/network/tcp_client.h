/*
  TcpClient: 异步 TCP 客户端，支持自动重连、消息回调和状态查询
  该类依赖于asio库, 下面是具体使用步骤:
------------------------------------------------------------------------------------------
  #include <iostream>
  #include <thread>

  #include "network/tcp_client.h"

  int main()
  {
    // 1. 创建 io_context，用于驱动异步操作
    asio::io_context io;

    // 2. 创建 TcpClient 实例，指定服务器 IP 和端口
    auto client = std::make_shared<TcpClient>(io, "127.0.0.1", "8080");

    // 3. 设置状态回调，用于监听连接状态、错误和重连信息
    client->set_status_callback(
      [](TcpClient::Status s, const std::string& info) { std::cout << "[Status] " << info << "\n"; });

    // 4. 设置消息回调，用于接收服务器发送的数据
    client->set_message_callback([](const std::string& msg) { std::cout << "[Recv] " << msg << "\n"; });

    // 5. 启动客户端，会立即尝试连接服务器并开启重连逻辑
    client->start();

    // 6. 在独立线程中运行 io_context，处理异步读写和重连
    std::thread t([&io] { io.run(); });

    // 7. 主线程发送消息给服务器
    std::string line;
    while (std::getline(std::cin, line))
    {
      if (line == "quit") break;  // 输入 quit 停止
      client->send(line);
    }

    // 8. 停止客户端，关闭连接并停止重连
    client->stop();

    // 9. 停止 io_context 并等待线程退出
    io.stop();
    t.join();
  }
------------------------------------------------------------------------------------------
*/



#pragma once
#include <array>
#include <asio.hpp>
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <string>

// TcpClient: 异步 TCP 客户端，支持自动重连、消息回调和状态查询
class TcpClient : public std::enable_shared_from_this<TcpClient>
{
 public:
  // 客户端连接状态
  enum class Status
  {
    Connected,     // 已连接
    Disconnected,  // 已断开
    Error,         // 出现错误
    Reconnecting   // 正在重连
  };

  using MessageCallback = std::function<void(const std::string&)>;         // 收到消息回调
  using StatusCallback = std::function<void(Status, const std::string&)>;  // 状态变化回调

  // 构造函数，传入io_context、服务器host和port
  TcpClient(asio::io_context& io, const std::string& host, const std::string& port);
  ~TcpClient() = default;

  // 启动客户端连接
  void start();

  // 停止客户端，关闭连接并取消重连
  void stop();

  // 设置收到消息时的回调
  void set_message_callback(MessageCallback cb);

  // 设置状态变化时的回调
  void set_status_callback(StatusCallback cb);

  // 线程安全状态查询
  Status get_status() const;

  // 发送数据（线程安全）
  void send(const std::string& msg);

 private:
  // 设置状态并触发状态回调
  void set_status(Status s, const std::string& info);

  // 执行连接操作
  void do_connect();

  // 计划重连（指数退避）
  void schedule_reconnect();

  // 异步读取数据
  void do_read();

  // 异步写入数据
  void do_write();

  // 关闭连接
  void close();

 private:
  asio::io_context& io_;                // ASIO IO上下文
  asio::ip::tcp::socket socket_;        // TCP套接字
  asio::steady_timer timer_;            // 重连定时器
  std::string host_, port_;             // 服务器地址和端口
  std::array<char, 1024> read_buf_;     // 读缓冲区
  std::deque<std::string> write_msgs_;  // 待发送消息队列

  std::atomic<Status> current_status_{Status::Disconnected};  // 当前状态
  std::atomic<bool> stopped_{true};                           // 是否已停止
  int reconnect_delay_ = 1;                                   // 重连延迟（秒）

  MessageCallback on_message_;  // 消息回调
  StatusCallback on_status_;    // 状态回调
};
