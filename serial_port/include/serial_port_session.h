#pragma once

#include <array>
#include <asio.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>

class SerialPortSession : public std::enable_shared_from_this<SerialPortSession>
{
 public:
  using ReceiveCallback = std::function<void(const std::string&)>;
  using ErrorCallback = std::function<void(const std::string&)>;

  static std::shared_ptr<SerialPortSession> create(const std::string& port_name, unsigned int baud_rate);

  void start();  // 👈 启动 io + open 串口
  void stop();   // 关闭串口 + 停止线程
  void send(std::string data);

  void set_receive_callback(ReceiveCallback cb);
  void set_error_callback(ErrorCallback cb);

  bool is_open() const;
  ~SerialPortSession();

 private:
  SerialPortSession(std::string port_name, unsigned int baud_rate);
  void open();  // 👈 私有，仅由 start() 调用
  void start_async_read();
  void report_info(const std::string& msg);
  void report_warn(const std::string& msg);
  void report_error(const std::string& msg);

  asio::io_context io_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  asio::strand<asio::io_context::executor_type> strand_;
  asio::serial_port serial_;
  std::thread io_thread_;
  std::atomic<bool> running_ = false;

  std::string port_name_;
  unsigned int baud_rate_ = 9600;

  std::array<char, 512> read_buffer_;
  ReceiveCallback receive_callback_;
  ErrorCallback error_callback_;
};
