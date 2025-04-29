#include <asio.hpp>
#include <iostream>
#include <memory>

using asio::ip::tcp;

void do_accept(tcp::acceptor& acceptor, asio::io_context& io_context);

void handle_client(std::shared_ptr<tcp::socket> socket)
{
  auto message = std::make_shared<std::string>("Hello from async server!\n");

  asio::async_write(*socket, asio::buffer(*message), [socket, message](std::error_code ec, std::size_t /*length*/) {
    if (!ec)
      std::cout << "Message sent to client\n";
    else
      std::cerr << "Write error: " << ec.message() << "\n";
  });
}

void do_accept(tcp::acceptor& acceptor, asio::io_context& io_context)
{
  auto socket = std::make_shared<tcp::socket>(io_context);

  acceptor.async_accept(*socket, [&, socket](std::error_code ec) {
    if (!ec)
    {
      std::cout << "Client connected\n";
      handle_client(socket);
    }
    else
    {
      std::cerr << "Accept error: " << ec.message() << "\n";
    }

    // 接受下一个连接
    do_accept(acceptor, io_context);
  });
}

int main()
{
  try
  {
    asio::io_context io_context;

    tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 12345));
    std::cout << "Server is listening on port 12345...\n";

    do_accept(acceptor, io_context);

    io_context.run();  // 启动事件循环
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
