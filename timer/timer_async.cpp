#include <asio.hpp>
#include <iostream>

/// @brief 用于timer的可调用对象
/// @param ec 错误码, timer.async_wait必须要这个error_code参数
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

// 传给async_wait的可调用对象, 必须接收一个 const std::error_code& 参数（当然也可以是按值传参 std::error_code，asio能接受）
struct Handler
{
  void operator()(const std::error_code& ec) const
  {
    if (!ec)
      std::cout << "Timer expired (functor)" << std::endl;
    else
      std::cout << "Error: " << ec.message() << std::endl;
  }
};

void testfunctor()
{
  asio::io_context io;
  asio::steady_timer timer(io, std::chrono::seconds(1));
  timer.async_wait(Handler{});
  io.run();
}

/// @brief 可重复的timer
void repeat_timer()
{
  asio::io_context io;
  asio::steady_timer timer(io, std::chrono::milliseconds(200));

  int count = 0;
  const int max_count = 10;  // 最多跑10次

  // 先声明，再定义
  std::function<void()> repeat;
  repeat = [&io, &timer, &count, max_count, &repeat]() {
    if (count >= max_count) return;
    std::cout << "Tick " << (count + 1) << std::endl;
    ++count;
    // 重置操作
    timer.expires_after(std::chrono::milliseconds(200));
    timer.async_wait([&io, &timer, &count, max_count, &repeat](const asio::error_code&) { repeat(); });
  };

  timer.async_wait([&io, &timer, &count, max_count, &repeat](const asio::error_code&) { repeat(); });
  io.run();
}

int main()
{
  std::cout << "=====main start=====" << std::endl;
  asio::io_context io;
  asio::steady_timer timer(io, std::chrono::seconds(2));
  timer.async_wait(&print);  // async_wait expects a callback with error_code
  std::cout << "timer.async_wait(print)" << std::endl;
  io.run();  // must call the asio::io_context::run()
  std::cout << "=====main end=====" << std::endl;
  testfunctor();
  repeat_timer();
}