#include <array>
#include <asio.hpp>
#include <functional>
#include <iostream>
#include <thread>

class SerialPortSession
{
 public:
  using ReceiveCallback = std::function<void(const std::string&)>;

  SerialPortSession(asio::io_context& io, const std::string& port_name, unsigned int baud_rate) :
    serial_(io), strand_(asio::make_strand(io)), read_buffer_{}
  {
    open(port_name, baud_rate);
  }

  void set_receive_callback(ReceiveCallback cb)
  {
    receive_callback_ = std::move(cb);
  }

  void send(const std::string& data)
  {
    asio::post(strand_, [this, data]() { asio::write(serial_, asio::buffer(data)); });
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
      start_async_read();
    }
    catch (const std::exception& e)
    {
      std::cerr << "[ERROR] Serial port open failed: " << e.what() << std::endl;
    }
  }

  void start_async_read()
  {
    serial_.async_read_some(
      asio::buffer(read_buffer_),
      asio::bind_executor(strand_, [this](const asio::error_code& ec, std::size_t bytes_transferred) {
        if (!ec)
        {
          std::string data(read_buffer_.data(), bytes_transferred);
          if (receive_callback_) receive_callback_(data);
          start_async_read();  // 继续监听
        }
        else
        {
          std::cerr << "[ERROR] Read failed: " << ec.message() << "\n";
        }
      }));
  }

  asio::serial_port serial_;
  asio::strand<asio::io_context::executor_type> strand_;
  std::array<char, 512> read_buffer_;
  ReceiveCallback receive_callback_;
};

int main()
{
  try
  {
    asio::io_context io;
    SerialPortSession session(io, "/dev/ttyS0", 9600);  // 修改为 COM3 或其他串口

    session.set_receive_callback([](const std::string& data) { std::cout << "[RECV] " << data; });

    std::thread io_thread([&]() { io.run(); });

    // 主线程负责从标准输入读取并发送
    std::string line;
    while (std::getline(std::cin, line))
    {
      session.send(line + "\n");
    }

    io_thread.join();
  }
  catch (const std::exception& e)
  {
    std::cerr << "[EXCEPTION] " << e.what() << "\n";
  }

  return 0;
}
