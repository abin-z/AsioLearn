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
