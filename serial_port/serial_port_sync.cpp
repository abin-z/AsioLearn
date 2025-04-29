#include <array>
#include <asio.hpp>
#include <iostream>
#include <thread>

class SerialPortSync
{
 public:
  SerialPortSync(asio::io_context& io, const std::string& port_name, unsigned int baud_rate) :
    serial_(io), port_name_(port_name)
  {
    open(port_name, baud_rate);
  }

  ~SerialPortSync()
  {
    if (serial_.is_open())
    {
      serial_.close();
    }
  }

  void send(const std::string& data)
  {
    asio::write(serial_, asio::buffer(data));
  }

  // 同步接收数据，阻塞式读取
  void receive()
  {
    while (true)
    {
      try
      {
        std::array<char, 512> read_buffer;
        std::size_t bytes_read = serial_.read_some(asio::buffer(read_buffer));
        std::string received_data(read_buffer.data(), bytes_read);
        std::cout << "[RECV] " << received_data;
      }
      catch (const std::exception& e)
      {
        std::cerr << "[ERROR] Serial port read failed: " << e.what() << "\n";
        break;
      }
    }
  }

 private:
  void open(const std::string& port_name, unsigned int baud_rate)
  {
    try
    {
      serial_.open(port_name);
      serial_.set_option(asio::serial_port_base::baud_rate(baud_rate));
      serial_.set_option(asio::serial_port_base::character_size(8));
      serial_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
      serial_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
      serial_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
    }
    catch (const std::exception& e)
    {
      std::cerr << "[ERROR] Failed to open serial port: " << e.what() << "\n";
      throw;
    }
  }

  asio::serial_port serial_;
  std::string port_name_;
};

int main()
{
  try
  {
    asio::io_context io;
    SerialPortSync serial(io, "/dev/ttyUSB0", 9600);  // Windows 下修改为 COM3 或其他串口

    // 启动接收线程
    std::thread receive_thread([&]() { serial.receive(); });

    // 主线程负责发送数据
    std::string line;
    while (std::getline(std::cin, line))
    {
      serial.send(line + "\n");
    }

    receive_thread.join();
  }
  catch (const std::exception& e)
  {
    std::cerr << "[EXCEPTION] " << e.what() << "\n";
  }

  return 0;
}
