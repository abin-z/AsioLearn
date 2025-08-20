#include "network/tcp_client.h"

#include <algorithm>
#include <chrono>

using asio::ip::tcp;

TcpClient::TcpClient(asio::io_context& io, const std::string& host, const std::string& port) :
  io_(io), socket_(io), timer_(io), host_(host), port_(port)
{
}

void TcpClient::start()
{
  stopped_.store(false);
  do_connect();
}

void TcpClient::stop()
{
  stopped_.store(true);
  close();
}

void TcpClient::set_message_callback(MessageCallback cb)
{
  on_message_ = std::move(cb);
}
void TcpClient::set_status_callback(StatusCallback cb)
{
  on_status_ = std::move(cb);
}
TcpClient::Status TcpClient::get_status() const
{
  return current_status_.load();
}

void TcpClient::send(const std::string& msg)
{
  auto self = shared_from_this();
  asio::post(io_, [this, self, msg] {
    if (stopped_.load()) return;
    bool writing = !write_msgs_.empty();
    write_msgs_.push_back(msg);
    if (!writing) do_write();
  });
}

void TcpClient::set_status(Status s, const std::string& info)
{
  current_status_.store(s);
  try
  {
    if (on_status_) on_status_(s, info);
  }
  catch (...)
  {
  }
}

void TcpClient::do_connect()
{
  if (stopped_.load()) return;
  tcp::resolver resolver(io_);
  auto endpoints = resolver.resolve(host_, port_);
  auto self = shared_from_this();
  asio::async_connect(socket_, endpoints, [this, self](std::error_code ec, tcp::endpoint) {
    if (stopped_.load()) return;
    if (!ec)
    {
      reconnect_delay_ = 1;
      set_status(Status::Connected, "Connected to server");
      do_read();
    }
    else
    {
      set_status(Status::Error, "Connect failed: " + ec.message());
      schedule_reconnect();
    }
  });
}

void TcpClient::schedule_reconnect()
{
  if (stopped_.load()) return;
  set_status(Status::Reconnecting, "Retry in " + std::to_string(reconnect_delay_) + "s");
  auto self = shared_from_this();
  timer_.expires_after(std::chrono::seconds(reconnect_delay_));
  timer_.async_wait([this, self](std::error_code ec) {
    if (!ec && !stopped_.load())
    {
      socket_ = tcp::socket(io_);
      do_connect();
      reconnect_delay_ = std::min(reconnect_delay_ * 2, 30);
    }
  });
}

void TcpClient::do_read()
{
  if (stopped_.load()) return;
  auto self = shared_from_this();
  socket_.async_read_some(asio::buffer(read_buf_), [this, self](std::error_code ec, std::size_t length) {
    if (stopped_.load()) return;
    if (!ec)
    {
      std::string msg(read_buf_.data(), length);
      try
      {
        if (on_message_) on_message_(msg);
      }
      catch (...)
      {
      }
      do_read();
    }
    else
    {
      if (ec == asio::error::eof || ec == asio::error::connection_reset)
        set_status(Status::Disconnected, "Server closed");
      else
        set_status(Status::Error, "Read error: " + ec.message());
      schedule_reconnect();
    }
  });
}

void TcpClient::do_write()
{
  if (stopped_.load()) return;
  auto self = shared_from_this();
  asio::async_write(socket_, asio::buffer(write_msgs_.front()), [this, self](std::error_code ec, std::size_t) {
    if (stopped_.load()) return;
    if (!ec)
    {
      write_msgs_.pop_front();
      if (!write_msgs_.empty()) do_write();
    }
    else
    {
      set_status(Status::Error, "Write error: " + ec.message());
      schedule_reconnect();
    }
  });
}

void TcpClient::close()
{
  auto self = shared_from_this();
  asio::post(io_, [this, self] {
    std::error_code ec;
    if (socket_.is_open())
    {
      socket_.shutdown(tcp::socket::shutdown_both, ec);
      socket_.close(ec);
      set_status(Status::Disconnected, "Client closed");
    }
  });
}
