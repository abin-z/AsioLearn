#include <asio.hpp>
#include <iostream>

void print(const std::error_code& ec)
{
  if (!ec)
  {
    std::cout << "asio::timer async" << std::endl;
  }
  else
  {
    std::cout << "Error occurred: " << ec.message() << std::endl;
  }
}

int main()
{
  std::cout << "=====main start=====" << std::endl;
  asio::io_context io;
  asio::steady_timer timer(io, std::chrono::seconds(3));
  timer.async_wait(print);  // async_wait expects a callback with error_code
  std::cout << "timer.async_wait(print)" << std::endl;
  io.run();  // must call the asio::io_context::run()
  std::cout << "=====main end=====" << std::endl;
}