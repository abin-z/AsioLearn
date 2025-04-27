#include <asio.hpp>
#include <iostream>

/// link: https://think-async.com/Asio/asio-1.30.2/doc/asio/tutorial/tuttimer1.html

int main()
{
  std::cout << "=====main start=====" << std::endl;
  asio::io_context io;
  asio::steady_timer timer(io, std::chrono::seconds(3));
  timer.wait();  // blocking wait
  std::cout << "asio::timer sync" << std::endl;
  std::cout << "=====main end=====" << std::endl;
}