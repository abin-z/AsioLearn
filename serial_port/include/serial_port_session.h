#pragma once

#include <array>
#include <asio.hpp>
#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <string>
#include <thread>

class SerialPortSession : public std::enable_shared_from_this<SerialPortSession>
{
 public:
  using ReceiveCallback = std::function<void(const std::string&)>;
  using ErrorCallback = std::function<void(const std::string&)>;

  static std::shared_ptr<SerialPortSession> create();

  void open(const std::string& port_name, unsigned int baud_rate);
  void stop();
  void send(std::string data);

  void set_receive_callback(ReceiveCallback cb);
  void set_error_callback(ErrorCallback cb);

  bool is_open() const;
  ~SerialPortSession();

 protected:
  SerialPortSession();

 private:
  void start_async_read();
  void report_info(const std::string& msg);
  void report_warn(const std::string& msg);
  void report_error(const std::string& msg);

  asio::io_context io_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  std::thread io_thread_;

  asio::strand<asio::io_context::executor_type> strand_;
  asio::serial_port serial_;

  std::array<char, 512> read_buffer_;
  ReceiveCallback receive_callback_;
  ErrorCallback error_callback_;

  std::atomic<bool> running_;
};
