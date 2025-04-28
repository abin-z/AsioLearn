#include <array>
#include <asio.hpp>
#include <iostream>

using asio::ip::udp;

int main(int argc, char* argv[])
{
  try
  {
    // 检查命令行参数是否正确，要求输入服务器主机名
    if (argc != 2)
    {
      std::cerr << "Usage: client <host>" << std::endl;
      return 1;
    }

    // 创建 io_context 对象，用于管理异步操作
    asio::io_context io_context;

    // 创建一个解析器用于将主机名解析为UDP端点
    udp::resolver resolver(io_context);
    udp::endpoint receiver_endpoint = *resolver.resolve(udp::v4(), argv[1], "daytime").begin();

    // 创建UDP socket
    udp::socket socket(io_context);
    socket.open(udp::v4());  // 打开socket，指定使用IPv4

    // 发送一个空白数据包到服务端（通常 daytime 服务只需要触发即可）
    std::array<char, 1> send_buf = {{0}};
    socket.send_to(asio::buffer(send_buf), receiver_endpoint);

    // 准备接收服务端返回的数据
    std::array<char, 128> recv_buf;
    udp::endpoint sender_endpoint;
    size_t len = socket.receive_from(asio::buffer(recv_buf), sender_endpoint);

    // 将接收到的数据写到标准输出
    std::cout.write(recv_buf.data(), len);
  }
  catch (std::exception& e)
  {
    // 捕获异常并输出错误信息
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
