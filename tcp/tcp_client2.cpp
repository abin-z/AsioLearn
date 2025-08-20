#include <iostream>
#include <thread>

#include "network/tcp_client.h"

int main()
{
  // 1. 创建 io_context，用于驱动异步操作
  asio::io_context io;

  // 2. 创建 TcpClient 实例，指定服务器 IP 和端口
  auto client = std::make_shared<TcpClient>(io, "127.0.0.1", "8080");

  // 3. 设置状态回调，用于监听连接状态、错误和重连信息
  client->set_status_callback(
    [](TcpClient::Status s, const std::string& info) { std::cout << "[Status] " << info << "\n"; });

  // 4. 设置消息回调，用于接收服务器发送的数据
  client->set_message_callback([](const std::string& msg) { std::cout << "[Recv] " << msg << "\n"; });

  // 5. 启动客户端，会立即尝试连接服务器并开启重连逻辑
  client->start();

  // 6. 在独立线程中运行 io_context，处理异步读写和重连
  std::thread t([&io] { io.run(); });

  // 7. 主线程发送消息给服务器
  std::string line;
  while (std::getline(std::cin, line))
  {
    if (line == "quit") break;  // 输入 quit 停止
    client->send(line);
  }

  // 8. 停止客户端，关闭连接并停止重连
  client->stop();

  // 9. 停止 io_context 并等待线程退出
  io.stop();
  t.join();
}
