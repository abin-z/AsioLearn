#include <asio.hpp>
#include <iostream>
#include <string>

int main()
{
  try
  {
    asio::io_context io;
    asio::serial_port serial(io);

    // 打开串口（Linux 示例为 /dev/ttyUSB0，Windows 用 "COM3"）
    serial.open("/dev/ttyS0");  // 请根据实际设备调整

    // 配置串口参数
    serial.set_option(asio::serial_port_base::baud_rate(9600));
    serial.set_option(asio::serial_port_base::character_size(8));
    serial.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    serial.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
    serial.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));

    // 写入数据
    std::string out_data = "Hello, serial!\n";
    asio::write(serial, asio::buffer(out_data));
    std::cout << "Sent: " << out_data;

    // 读取返回数据
    char buf[128];
    std::size_t len = serial.read_some(asio::buffer(buf));
    std::string received(buf, len);
    std::cout << "Received: " << received << std::endl;

    serial.close();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
