#include <asio.hpp>
#include <iostream>

int main()
{
  std::cout << "=====main start=====" << std::endl;
  asio::io_context io;
  asio::steady_timer timer(io, std::chrono::seconds(3));
  timer.wait();  // blocking wait
  std::cout << "asio::timer sync" << std::endl;
  std::cout << "=====main end=====" << std::endl;
}