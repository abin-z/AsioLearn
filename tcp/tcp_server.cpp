#include <asio.hpp>
#include <ctime>
#include <iostream>
#include <string>

using asio::ip::tcp;

// 生成当前系统时间的字符串
std::string make_daytime_string()
{
  using namespace std;   // 引入time_t, time, ctime等
  time_t now = time(0);  // 获取当前时间戳
  return ctime(&now);    // 转成字符串格式
}

int main()
{
  try
  {
    asio::io_context io_context;  // IO上下文，所有IO操作都靠它驱动

    // 创建TCP服务器，监听所有IPv4地址的13端口
    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 13));
    // 获取并打印本地绑定的IP地址和端口
    tcp::endpoint local_endpoint = acceptor.local_endpoint();
    std::cout << "TCP server started on " << local_endpoint.address().to_string()  // 获取本地IP地址
              << ":" << local_endpoint.port() << std::endl;                        // 获取本地端口号

    for (;;)
    {
      tcp::socket socket(io_context);  // 每次循环新建一个socket对象
      acceptor.accept(socket);         // 阻塞等待客户端连接

      std::string message = make_daytime_string();  // 获取当前时间字符串

      std::error_code error_code;
      asio::write(socket, asio::buffer(message), error_code);  // 发送时间给客户端
      // 处理error_code, 判断是否发送成功
      if (error_code)
      {
        std::cerr << "Send failed: " << error_code.message() << std::endl;
      }
      else
      {
        std::cout << "Send success." << std::endl;
      }
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;  // 捕获并输出异常
  }

  return 0;
}
