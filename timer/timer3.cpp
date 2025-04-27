#include <asio.hpp>
#include <functional>
#include <iostream>

// 参考文档链接：https://think-async.com/Asio/asio-1.30.2/doc/asio/tutorial/tuttimer3.html

// 定义一个打印函数，作为定时器回调
// 该函数在定时器触发时被调用，并且会打印当前计数值
// 递增计数值并重新设置定时器，确保定时器继续工作
void print(const std::error_code& /*e*/, asio::steady_timer* t, int* count)
{
  // 如果计数值小于5，继续执行
  if (*count < 5)
  {
    std::cout << *count << std::endl;  // 打印当前计数值
    ++(*count);  // 增加计数值

    // 重设定时器时间，延迟1秒
    t->expires_at(t->expiry() + asio::chrono::seconds(1));

    // 设置定时器的异步等待，再次调用print函数
    t->async_wait(std::bind(print, asio::placeholders::error, t, count));
  }
}

int main()
{
  // 创建io_context对象，负责调度和执行异步操作
  asio::io_context io;

  int count = 0;
  asio::steady_timer t(io, asio::chrono::seconds(1));

  // 设置定时器的异步等待，等待定时器触发时调用print函数
  t.async_wait(std::bind(print, asio::placeholders::error, &t, &count));

  // 启动io_context，开始执行异步操作
  io.run();

  // 打印最终的计数值（此时应该是5）
  std::cout << "Final count is " << count << std::endl;

  return 0;
}
