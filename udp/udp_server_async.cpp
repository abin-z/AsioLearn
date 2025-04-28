#include <array>
#include <asio.hpp>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

using asio::ip::udp;

// 生成当前时间的字符串
std::string make_daytime_string()
{
  using namespace std;   // 使用标准命名空间中的 time_t, time, ctime
  time_t now = time(0);  // 获取当前系统时间
  return ctime(&now);    // 转换为可读的时间字符串（自带换行）
}

// 定义 UDP 服务器类
class udp_server
{
 public:
  // 构造函数，初始化 socket 并绑定到本地13号端口
  udp_server(asio::io_context& io_context) : socket_(io_context, udp::endpoint(udp::v4(), 13))
  {
    start_receive();  // 启动第一次异步接收
  }

 private:
  // 开始异步接收数据
  void start_receive()
  {
    socket_.async_receive_from(
      asio::buffer(recv_buffer_), remote_endpoint_,
      std::bind(&udp_server::handle_receive, this, asio::placeholders::error, asio::placeholders::bytes_transferred));
  }

  // 异步接收完成后的处理函数
  void handle_receive(const std::error_code& error, std::size_t /*bytes_transferred*/)
  {
    if (!error)
    {
      // 收到请求后，生成当前时间字符串
      std::shared_ptr<std::string> message(new std::string(make_daytime_string()));

      // 异步发送时间字符串给客户端
      socket_.async_send_to(asio::buffer(*message), remote_endpoint_,
                            std::bind(&udp_server::handle_send, this, message, asio::placeholders::error,
                                      asio::placeholders::bytes_transferred));

      // 继续等待下一个请求（循环收发）
      start_receive();
    }
    // 如果有错误，直接忽略（可以扩展：打印日志或处理错误）
  }

  // 异步发送完成后的处理函数（此处不做任何处理）
  void handle_send(std::shared_ptr<std::string> /*message*/, const std::error_code& /*error*/,
                   std::size_t /*bytes_transferred*/)
  {
    // 发送完成后，这里什么也不做。
    // 注意：message 是 shared_ptr，确保异步发送时消息内容有效。
  }

  udp::socket socket_;               // UDP socket，用于通信
  udp::endpoint remote_endpoint_;    // 记录远程客户端地址
  std::array<char, 1> recv_buffer_;  // 接收缓冲区（这里只需要一点数据触发即可）
};

int main()
{
  try
  {
    // 创建 io_context，管理异步操作
    asio::io_context io_context;

    // 创建并运行UDP服务器
    udp_server server(io_context);

    // 运行事件循环，处理所有异步事件
    io_context.run();
  }
  catch (std::exception& e)
  {
    // 捕获异常并打印错误信息
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
