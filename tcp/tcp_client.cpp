#include <array>
#include <asio.hpp>
#include <iostream>

// 运行示例: ./tcp_client time.nist.gov

using asio::ip::tcp;

int main(int argc, char* argv[])
{
  try
  {
    // 检查参数，要求提供服务器地址, 例如: time.nist.gov
    if (argc != 2)
    {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }

    asio::io_context io_context;  // IO上下文对象，管理异步操作

    tcp::resolver resolver(io_context);  // 域名解析器
    // auto endpoints = resolver.resolve("192.168.1.100", "8080");
    tcp::resolver::results_type endpoints = resolver.resolve(argv[1], "daytime");  // 解析主机名和服务

    tcp::socket socket(io_context);    // TCP套接字
    asio::connect(socket, endpoints);  // 连接到服务器

    for (;;)
    {
      std::array<char, 128> buf;  // 读缓冲区
      std::error_code error;      // 错误码

      size_t len = socket.read_some(asio::buffer(buf), error);  // 阻塞读取数据

      if (error == asio::error::eof)
        break;  // 对方关闭连接，退出循环
      else if (error)
        throw std::system_error(error);  // 发生其他错误，抛出异常

      std::cout.write(buf.data(), len);  // 输出收到的数据
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;  // 捕获并打印异常信息
  }

  return 0;
}
