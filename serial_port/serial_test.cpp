#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "serial_port_session.h"

int main()
{

  auto ports = SerialPortSession::list_serial_ports();
  if (ports.empty())
  {
    std::cout << "No serial ports found.\n";
  }
  else
  {
    std::cout << "Available serial ports:\n";
    for (const auto& port : ports)
    {
      std::cout << "  " << port << "\n";
    }
  }

  std::string port = "COM2";
  unsigned int baud = 115200;

  auto session = SerialPortSession::create(port, baud);
  session->stop();
  session = SerialPortSession::create(port, baud);
  std::cout << "session->is_open()1 = " << session->is_open();
  session->set_receive_callback([](const std::string& data) { std::cout << "[RECEIVED] " << data << std::endl; });

  session->set_error_callback([](const std::string& msg) { std::cerr << msg << std::endl; });

  // session->send("hello");
  auto ret = session->start();  // 启动串口会话
  // auto ret = false;
  std::cout << "Type to send data to serial port. Type 'exit' to quit." + std::to_string(ret) + "\n";
  // session->stop();
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  // ret = session->start();  // 启动串口会话
  // std::cout << "Type to send data to serial port. Type 'exit' to quit2." + std::to_string(ret) + "\n";
  std::cout << "session->is_open()2 = " << session->is_open();
  session->send("hhhhhhhhhhhhhhhhhhhhhhhhh");
  session->stop();
  // session.reset();
  session = SerialPortSession::create(port, baud);

  session->set_receive_callback([](const std::string& data) { std::cout << "[RECEIVED2] " << data << std::endl; });
  session->set_error_callback([](const std::string& msg) { std::cerr << msg << std::endl; });
  std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 目前关闭后需要延时
  session->start();
  session->send("yyyyyyyyyyyyyyyyyyyyyyyy");

  std::string line;
  while (true)
  {
    std::getline(std::cin, line);
    if (line == "exit") break;

    session->send(line);
  }
  std::cout << "Serial port session stopped.\n";
  return 0;
}
