#include <array>
#include <asio.hpp>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

using asio::ip::tcp;
using asio::ip::udp;

// 生成当前时间的字符串
std::string make_daytime_string()
{
  using namespace std;   // 使用标准命名空间里的 time_t、time、ctime
  time_t now = time(0);  // 获取当前时间戳
  return ctime(&now);    // 转成可读的字符串（注意带换行）
}

// 表示一个 TCP 连接，负责发送 daytime 消息
class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{
 public:
  typedef std::shared_ptr<tcp_connection> pointer;  // 定义智能指针类型

  // 工厂函数，创建一个 tcp_connection 实例
  static pointer create(asio::io_context& io_context)
  {
    return pointer(new tcp_connection(io_context));
  }

  // 获取底层 socket 引用
  tcp::socket& socket()
  {
    return socket_;
  }

  // 启动连接，发送 daytime 消息
  void start()
  {
    message_ = make_daytime_string();  // 生成要发送的时间字符串

    // 异步写入消息到 socket
    asio::async_write(socket_, asio::buffer(message_), std::bind(&tcp_connection::handle_write, shared_from_this()));
  }

 private:
  // 构造函数，初始化 socket
  tcp_connection(asio::io_context& io_context) : socket_(io_context) {}

  // 异步写完成后的处理（这里暂时不做任何事）
  void handle_write() {}

  tcp::socket socket_;   // TCP socket
  std::string message_;  // 要发送的时间消息
};

// TCP 服务器，监听端口并接收连接
class tcp_server
{
 public:
  // 构造，初始化 acceptor 监听本地 13 端口
  tcp_server(asio::io_context& io_context) :
    io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 13))
  {
    start_accept();  // 启动第一次异步接受连接
  }

 private:
  // 启动异步接受新连接
  void start_accept()
  {
    tcp_connection::pointer new_connection = tcp_connection::create(io_context_);

    acceptor_.async_accept(new_connection->socket(),
                           std::bind(&tcp_server::handle_accept, this, new_connection, asio::placeholders::error));
  }

  // 连接接受完成后的处理
  void handle_accept(tcp_connection::pointer new_connection, const std::error_code& error)
  {
    if (!error)
    {
      new_connection->start();  // 启动连接（发送消息）
    }

    start_accept();  // 继续等待下一个连接
  }

  asio::io_context& io_context_;  // 引用 io_context
  tcp::acceptor acceptor_;        // TCP 连接接受器
};

// UDP 服务器，监听端口并响应请求
class udp_server
{
 public:
  // 构造，初始化 socket 绑定到本地 13 端口
  udp_server(asio::io_context& io_context) : socket_(io_context, udp::endpoint(udp::v4(), 13))
  {
    start_receive();  // 启动第一次异步接收
  }

 private:
  // 启动异步接收数据
  void start_receive()
  {
    socket_.async_receive_from(asio::buffer(recv_buffer_), remote_endpoint_,
                               std::bind(&udp_server::handle_receive, this, asio::placeholders::error));
  }

  // 接收完成后的处理
  void handle_receive(const std::error_code& error)
  {
    if (!error)
    {
      std::shared_ptr<std::string> message(new std::string(make_daytime_string()));  // 准备回复的消息

      // 异步发送回应
      socket_.async_send_to(asio::buffer(*message), remote_endpoint_,
                            std::bind(&udp_server::handle_send, this, message));

      start_receive();  // 继续等待下一次接收
    }
    // 错误的话直接忽略（可以扩展成日志）
  }

  // 发送完成后的处理（这里不需要做什么）
  void handle_send(std::shared_ptr<std::string> /*message*/) {}

  udp::socket socket_;               // UDP socket
  udp::endpoint remote_endpoint_;    // 记录远程端地址
  std::array<char, 1> recv_buffer_;  // 接收缓冲区（只需要触发收到数据）
};

// 程序入口
int main()
{
  try
  {
    asio::io_context io_context;  // 创建 io_context

    tcp_server server1(io_context);  // 创建 TCP 服务器
    udp_server server2(io_context);  // 创建 UDP 服务器

    io_context.run();  // 启动事件循环，处理所有异步操作
  }
  catch (std::exception& e)
  {
    // 捕获异常并打印错误信息
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
