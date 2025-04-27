#include <asio.hpp>
#include <ctime>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

using asio::ip::tcp;

// 获取当前日期和时间的字符串
std::string make_daytime_string()
{
  using namespace std;   // For time_t, time and ctime;
  time_t now = time(0);  // 获取当前时间
  return ctime(&now);    // 转换为字符串
}

// TCP 连接类，管理与客户端的连接
class tcp_connection : public std::enable_shared_from_this<tcp_connection>  // 支持从自身获取 shared_ptr
{
 public:
  typedef std::shared_ptr<tcp_connection> pointer;

  // 创建一个新的 tcp_connection 实例
  static pointer create(asio::io_context& io_context)
  {
    return pointer(new tcp_connection(io_context));
  }

  // 返回 socket，用于与客户端通信
  tcp::socket& socket()
  {
    return socket_;
  }

  // 启动连接，发送当前时间给客户端
  void start()
  {
    message_ = make_daytime_string();  // 获取当前时间字符串

    // 异步写数据到客户端
    asio::async_write(socket_, asio::buffer(message_),
                      std::bind(&tcp_connection::handle_write, shared_from_this(), asio::placeholders::error,
                                asio::placeholders::bytes_transferred));
  }

 private:
  // 私有构造函数，只能通过 create() 创建实例
  tcp_connection(asio::io_context& io_context) : socket_(io_context)  // 初始化 socket
  {
  }

  // 异步写完成后的回调函数
  void handle_write(const std::error_code& /*error*/, size_t /*bytes_transferred*/)
  {
    // 这里可以处理写操作完成后的逻辑，比如关闭连接等
  }

  tcp::socket socket_;   // 与客户端的连接 socket
  std::string message_;  // 要发送的消息
};

// TCP 服务器类，负责接受客户端连接并处理
class tcp_server
{
 public:
  // 构造函数，初始化服务器并开始监听 13 端口
  tcp_server(asio::io_context& io_context) :
    io_context_(io_context), acceptor_(io_context, tcp::endpoint(tcp::v4(), 13))  // 监听端口 13
  {
    start_accept();  // 启动接受连接的操作
  }

 private:
  // 启动异步接受连接
  void start_accept()
  {
    // 创建一个新的连接对象
    tcp_connection::pointer new_connection = tcp_connection::create(io_context_);

    // 异步接受连接
    acceptor_.async_accept(new_connection->socket(),
                           std::bind(&tcp_server::handle_accept, this, new_connection, asio::placeholders::error));
  }

  // 连接接受完成后的回调函数
  void handle_accept(tcp_connection::pointer new_connection, const std::error_code& error)
  {
    if (!error)  // 如果没有错误，启动连接
    {
      new_connection->start();
    }

    // 继续接受下一个连接
    start_accept();
  }

  asio::io_context& io_context_;  // io_context 用于驱动异步操作
  tcp::acceptor acceptor_;        // 用于接受客户端连接的 acceptor
};

int main()
{
  try
  {
    asio::io_context io_context;    // 创建 io_context，处理异步事件
    tcp_server server(io_context);  // 创建 TCP 服务器
    io_context.run();               // 启动事件循环，处理异步操作
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << std::endl;  // 异常处理
  }

  return 0;
}
