#pragma once
#include <array>
#include <asio.hpp>
#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <string>

class TcpClient : public std::enable_shared_from_this<TcpClient>
{
 public:
  enum class Status
  {
    Connected,
    Disconnected,
    Error,
    Reconnecting
  };

  using MessageCallback = std::function<void(const std::string&)>;
  using StatusCallback = std::function<void(Status, const std::string&)>;

  TcpClient(asio::io_context& io, const std::string& host, const std::string& port);
  ~TcpClient() = default;

  void start();
  void stop();

  void set_message_callback(MessageCallback cb);
  void set_status_callback(StatusCallback cb);

  // 线程安全状态查询
  Status get_status() const;

  void send(const std::string& msg);

 private:
  void set_status(Status s, const std::string& info);
  void do_connect();
  void schedule_reconnect();
  void do_read();
  void do_write();
  void close();

 private:
  asio::io_context& io_;
  asio::ip::tcp::socket socket_;
  asio::steady_timer timer_;
  std::string host_, port_;
  std::array<char, 1024> read_buf_;
  std::deque<std::string> write_msgs_;

  std::atomic<Status> current_status_{Status::Disconnected};
  std::atomic<bool> stopped_{true};
  int reconnect_delay_ = 1;

  MessageCallback on_message_;
  StatusCallback on_status_;
};
