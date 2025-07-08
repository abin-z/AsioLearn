#include "serial_port_session.h"

SerialPortSession::SerialPortSession() :
  work_guard_(asio::make_work_guard(io_)), strand_(asio::make_strand(io_)), serial_(io_)
{
  io_thread_ = std::thread([this]() { io_.run(); });
}

SerialPortSession::~SerialPortSession()
{
  stop();
  if (io_thread_.joinable()) io_thread_.join();
}

std::shared_ptr<SerialPortSession> SerialPortSession::create()
{
  return std::shared_ptr<SerialPortSession>(new SerialPortSession());
}

void SerialPortSession::set_receive_callback(ReceiveCallback cb)
{
  receive_callback_ = std::move(cb);
}

void SerialPortSession::set_error_callback(ErrorCallback cb)
{
  error_callback_ = std::move(cb);
}

bool SerialPortSession::is_open() const
{
  return serial_.is_open();
}

void SerialPortSession::open(const std::string& port_name, unsigned int baud_rate)
{
  auto self = shared_from_this();
  asio::post(strand_, [self, port_name, baud_rate]() {
    try
    {
      self->serial_.open(port_name);
      self->serial_.set_option(asio::serial_port_base::baud_rate(baud_rate));
      self->serial_.set_option(asio::serial_port_base::character_size(8));
      self->serial_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
      self->serial_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
      self->serial_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));
      self->report_info("Serial port opened: " + port_name);
      self->start_async_read();
    }
    catch (const std::exception& e)
    {
      self->report_error(std::string("Failed to open serial port: ") + e.what());
    }
  });
}

void SerialPortSession::stop()
{
  auto self = shared_from_this();
  asio::post(strand_, [self]() {
    if (self->serial_.is_open())
    {
      asio::error_code ec;
      asio::error_code cancel_ec = self->serial_.cancel(ec);
      if (cancel_ec)
      {
        self->report_error("Serial port cancel failed: " + cancel_ec.message());
      }
      asio::error_code close_ec = self->serial_.close(ec);
      if (close_ec)
      {
        self->report_error("Serial port close failed: " + close_ec.message());
      }
      self->report_info("Serial port closed.");
    }
  });
}

void SerialPortSession::send(const std::string& data)
{
  auto self = shared_from_this();
  asio::post(strand_, [self, data]() {
    if (!self->serial_.is_open()) return;
    asio::async_write(self->serial_, asio::buffer(data),
                      asio::bind_executor(self->strand_, [self](const asio::error_code& ec, std::size_t) {
                        if (ec) self->report_error("Send error: " + ec.message());
                      }));
  });
}

void SerialPortSession::start_async_read()
{
  auto self = shared_from_this();
  self->serial_.async_read_some(
    asio::buffer(self->read_buffer_),
    asio::bind_executor(self->strand_, [self](const asio::error_code& ec, std::size_t bytes_transferred) {
      if (!ec)
      {
        std::string data(self->read_buffer_.data(), bytes_transferred);
        if (self->receive_callback_) self->receive_callback_(data);
        self->start_async_read();  // 继续监听
      }
      else
      {
        self->report_error("Read error: " + ec.message());
      }
    }));
}

void SerialPortSession::report_info(const std::string& msg)
{
  if (error_callback_) error_callback_("[INFO] " + msg);
}

void SerialPortSession::report_warn(const std::string& msg)
{
  if (error_callback_) error_callback_("[WARN] " + msg);
}

void SerialPortSession::report_error(const std::string& msg)
{
  if (error_callback_) error_callback_("[ERROR] " + msg);
}
