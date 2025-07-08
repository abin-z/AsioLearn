/*
使用示例：基本串口通信流程

#include "serial_port_session.h"
#include <iostream>

int main()
{
  // 1. 创建串口会话，指定串口号和波特率
  auto session = SerialPortSession::create("COM1", 115200);

  // 2. 设置接收数据回调
  session->set_receive_callback([](const std::string& data) {
    std::cout << "[RECEIVED] " << data << std::endl;
  });

  // 3. 设置错误回调（如串口断开、读取失败等）
  session->set_error_callback([](const std::string& err) {
    std::cerr << "[ERROR] " << err << std::endl;
  });

  // 4. 启动串口通信（打开串口 + 启动异步读写线程）
  session->start();

  // 5. 向串口发送数据
  session->send("Hello, Serial Port!");

  // 6. 关闭串口（可选，析构时会自动关闭）
  session->stop();

  return 0;
}
*/

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

  // 禁用拷贝和赋值
  SerialPortSession(const SerialPortSession&) = delete;
  SerialPortSession& operator=(const SerialPortSession&) = delete;

  static std::shared_ptr<SerialPortSession> create(const std::string& port_name, unsigned int baud_rate);

  void start();  // 启动 io + open 串口
  void stop();   // 关闭串口 + 停止线程
  void send(std::string data);

  void set_receive_callback(ReceiveCallback cb);
  void set_error_callback(ErrorCallback cb);

  bool is_open() const;
  ~SerialPortSession();

 private:
  SerialPortSession(std::string port_name, unsigned int baud_rate);
  void open();
  void start_async_read();
  void report_info(const std::string& msg);
  void report_warn(const std::string& msg);
  void report_error(const std::string& msg);

  asio::io_context io_;
  asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
  asio::strand<asio::io_context::executor_type> strand_;
  asio::serial_port serial_;
  std::thread io_thread_;
  std::atomic<bool> running_{false};

  std::string port_name_;
  unsigned int baud_rate_{9600};

  std::array<char, 1024> read_buffer_;
  ReceiveCallback receive_callback_;
  ErrorCallback error_callback_;
};
