#include <asio.hpp>
#include <functional>
#include <iostream>
#include <thread>

// 参考文档链接：https://think-async.com/Asio/asio-1.30.2/doc/asio/tutorial/tuttimer5.html

class printer
{
 public:
  // 构造函数
  // 初始化定时器、strand和计数器
  printer(asio::io_context& io) :
    // 使用strand，确保任务在同一个线程中按顺序执行
    strand_(asio::make_strand(io)),
    // 定义两个定时器，分别每秒触发一次
    timer1_(io, asio::chrono::seconds(1)),
    timer2_(io, asio::chrono::seconds(1)),
    count_(0)  // 初始化计数器
  {
    // 定时器1的异步等待，触发时调用打印函数print1
    timer1_.async_wait(asio::bind_executor(strand_, std::bind(&printer::print1, this)));

    // 定时器2的异步等待，触发时调用打印函数print2
    timer2_.async_wait(asio::bind_executor(strand_, std::bind(&printer::print2, this)));
  }

  // 析构函数，输出最终的计数值
  ~printer()
  {
    std::cout << "Final count is " << count_ << std::endl;
  }

  // 打印定时器1的输出，并递增计数器
  // 每次执行后，重置定时器，继续异步等待
  void print1()
  {
    if (count_ < 10)  // 如果计数器小于10，继续打印
    {
      std::cout << "Timer 1: " << count_ << std::endl;
      ++count_;  // 计数器加1

      // 重置定时器的时间，继续等待下一次触发
      timer1_.expires_at(timer1_.expiry() + asio::chrono::seconds(1));

      // 异步等待定时器1再次触发，继续执行print1
      timer1_.async_wait(asio::bind_executor(strand_, std::bind(&printer::print1, this)));
    }
  }

  // 打印定时器2的输出，并递增计数器
  // 每次执行后，重置定时器，继续异步等待
  void print2()
  {
    if (count_ < 10)  // 如果计数器小于10，继续打印
    {
      std::cout << "Timer 2: " << count_ << std::endl;
      ++count_;  // 计数器加1

      // 重置定时器的时间，继续等待下一次触发
      timer2_.expires_at(timer2_.expiry() + asio::chrono::seconds(1));

      // 异步等待定时器2再次触发，继续执行print2
      timer2_.async_wait(asio::bind_executor(strand_, std::bind(&printer::print2, this)));
    }
  }

 private:
  // 用于确保定时器的回调按顺序执行，即便有多个线程在运行
  asio::strand<asio::io_context::executor_type> strand_;

  // 定时器1和定时器2，每隔1秒钟触发一次
  asio::steady_timer timer1_;
  asio::steady_timer timer2_;

  // 计数器
  int count_;
};

int main()
{
  // 创建io_context对象，负责执行异步操作
  asio::io_context io;

  // 创建printer对象，它会启动定时器并开始定时任务
  printer p(io);

  // 创建一个新线程运行io_context的事件循环（处理异步任务）
  std::thread t([&] { io.run(); });

  // 在主线程中再次运行io_context，保证所有任务都能完成
  io.run();

  // 等待子线程结束
  t.join();

  return 0;
}
