#include <iostream>
#include <string>

#include "serial_port_session.h"


int main()
{
  std::string port = "COM2";
  unsigned int baud = 115200;

  auto session = SerialPortSession::create(port, baud);

  session->set_receive_callback([](const std::string& data) { std::cout << "[RECEIVED] " << data << std::endl; });

  session->set_error_callback([](const std::string& msg) { std::cerr << msg << std::endl; });

  session->start();  // 启动串口会话

  std::cout << "Type to send data to serial port. Type 'exit' to quit.\n";

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
