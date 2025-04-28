#include <array>
#include <asio.hpp>
#include <ctime>
#include <iostream>
#include <string>

using asio::ip::udp;

// 生成当前时间的字符串
std::string make_daytime_string()
{
  using namespace std;   // 使用标准命名空间中的 time_t、time、ctime
  time_t now = time(0);  // 获取当前系统时间
  return ctime(&now);    // 转换成可读字符串（自带换行）
}

int main()
{
  try
  {
    // 创建 io_context 对象，管理异步操作
    asio::io_context io_context;

    // 创建一个UDP socket，绑定到本地13号端口（daytime协议标准端口）
    udp::socket socket(io_context, udp::endpoint(udp::v4(), 13));

    // 无限循环，持续接收请求并回复
    for (;;)
    {
      // 接收缓冲区，只需要接收1字节（因为客户端只是为了触发请求）
      std::array<char, 1> recv_buf;
      udp::endpoint remote_endpoint;  // 用于存放发送方的地址信息
      socket.receive_from(asio::buffer(recv_buf), remote_endpoint);

      // 生成当前时间字符串作为回应消息
      std::string message = make_daytime_string();

      // 将时间字符串发送回客户端
      std::error_code ignored_error;
      socket.send_to(asio::buffer(message), remote_endpoint, 0, ignored_error);
    }
  }
  catch (std::exception& e)
  {
    // 捕获异常并输出错误信息
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
