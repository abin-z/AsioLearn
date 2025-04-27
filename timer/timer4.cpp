#include <asio.hpp>
#include <functional>
#include <iostream>

// 参考文档链接：https://think-async.com/Asio/asio-1.30.2/doc/asio/tutorial/tuttimer4.html

class printer
{
 public:
  // 构造函数：初始化定时器，并设置异步等待
  // 定时器每隔1秒触发一次
  printer(asio::io_context& io) : timer_(io, asio::chrono::seconds(1)), count_(0)
  {
    // 设置定时器的异步等待，触发时调用print方法
    timer_.async_wait(std::bind(&printer::print, this));
  }

  // 析构函数：在对象销毁时输出最终的计数
  ~printer()
  {
    std::cout << "Final count is " << count_ << std::endl;
  }

  // 定时器回调函数：每次触发时打印当前计数并递增
  // 当计数小于5时，继续设置定时器并递增计数
  void print()
  {
    if (count_ < 5)  // 如果计数小于5，则继续执行
    {
      std::cout << count_ << std::endl;  // 打印当前计数
      ++count_;  // 递增计数

      // 重置定时器的触发时间，每次延迟1秒
      timer_.expires_at(timer_.expiry() + asio::chrono::seconds(1));
      
      // 设置定时器的异步等待，继续执行print方法
      timer_.async_wait(std::bind(&printer::print, this));
    }
  }

 private:
  // 定时器对象，初始化时每秒触发一次
  asio::steady_timer timer_;
  
  // 计数器，用于追踪定时器触发的次数
  int count_;
};

int main()
{
  // 创建io_context对象，负责执行异步操作
  asio::io_context io;
  
  // 创建printer对象，它会启动定时器并开始定时任务
  printer p(io);
  
  // 运行io_context，开始处理异步事件
  io.run();

  return 0;
}
