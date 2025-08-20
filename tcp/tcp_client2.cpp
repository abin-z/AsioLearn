#include "network/tcp_client.h"
#include <iostream>
#include <thread>

int main() {
    asio::io_context io;
    auto client = std::make_shared<TcpClient>(io, "127.0.0.1", "8080");

    client->set_status_callback([](auto s, const std::string& info){
        std::cout << "[Status] " << info << "\n";
    });

    client->set_message_callback([](const std::string& msg){
        std::cout << "[Recv] " << msg << "\n";
    });

    client->start();

    std::thread t([&io]{ io.run(); });

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        client->send(line);
    }

    client->stop();
    io.stop();
    t.join();
}
