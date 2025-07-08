#include <iostream>

#include "serial_port_session.h"

int main()
{
  // 创建串口会话实例
  auto session = SerialPortSession::create();

  // 设置接收回调，打印收到的数据
  session->set_receive_callback([](const std::string& data) { std::cout << "Received: " << data << std::endl; });

  // 设置错误回调，打印错误信息
  session->set_error_callback([](const std::string& err) { std::cerr << err << std::endl; });

  // 打开串口，修改为你实际的串口名称和波特率
  session->open("COM3", 115200);

  
  session->send("hello abin\n");
  
  std::cout << "Enter text to send, empty line to quit:" << std::endl;

  return 0;
}
