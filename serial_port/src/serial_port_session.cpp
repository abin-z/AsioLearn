#include "serial_port_session.h"

#include <future>

SerialPortSession::SerialPortSession(std::string port_name, unsigned int baud_rate) :
  work_guard_(asio::make_work_guard(io_)),
  strand_(asio::make_strand(io_)),
  serial_(io_),
  port_name_(std::move(port_name)),
  baud_rate_(baud_rate)
{
}

SerialPortSession::~SerialPortSession()
{
  stop();  // RAII
}

std::shared_ptr<SerialPortSession> SerialPortSession::create(const std::string& port_name, unsigned int baud_rate)
{
  return std::shared_ptr<SerialPortSession>(new SerialPortSession(port_name, baud_rate));
}

void SerialPortSession::start()
{
  if (running_) return;
  running_ = true;

  open();  // 👈 在 start() 里打开串口

  io_thread_ = std::thread([this]() { io_.run(); });
}

void SerialPortSession::open()
{
  try
  {
    serial_.open(port_name_);
    serial_.set_option(asio::serial_port_base::baud_rate(baud_rate_));
    serial_.set_option(asio::serial_port_base::character_size(8));
    serial_.set_option(asio::serial_port_base::parity(asio::serial_port_base::parity::none));
    serial_.set_option(asio::serial_port_base::stop_bits(asio::serial_port_base::stop_bits::one));
    serial_.set_option(asio::serial_port_base::flow_control(asio::serial_port_base::flow_control::none));

    report_info("Serial port opened: " + port_name_);
    start_async_read();
  }
  catch (const std::exception& e)
  {
    report_error("Failed to open serial port: " + std::string(e.what()));
  }
}

void SerialPortSession::stop()
{
  if (!running_) return;
  running_ = false;

  auto self = shared_from_this();
  std::promise<void> done;
  auto fut = done.get_future();

  asio::post(strand_, [self, &done]() {
    if (self->serial_.is_open())
    {
      asio::error_code ec;
      self->serial_.cancel(ec);
      self->serial_.close(ec);
      self->report_info("Serial port closed.");
    }
    done.set_value();
  });

  fut.wait();

  io_.stop();
  if (io_thread_.joinable()) io_thread_.join();
}

void SerialPortSession::send(std::string data)
{
  if (!running_) return;

  auto self = shared_from_this();
  auto data_ptr = std::make_shared<std::string>(std::move(data));

  asio::post(strand_, [self, data_ptr]() {
    if (!self->serial_.is_open()) return;

    asio::async_write(self->serial_, asio::buffer(*data_ptr),
                      asio::bind_executor(self->strand_, [self, data_ptr](const asio::error_code& ec, std::size_t) {
                        if (ec && ec != asio::error::operation_aborted)
                          self->report_error("Send error: " + ec.message());
                      }));
  });
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

void SerialPortSession::start_async_read()
{
  auto self = shared_from_this();
  serial_.async_read_some(
    asio::buffer(read_buffer_),
    asio::bind_executor(strand_, [self](const asio::error_code& ec, std::size_t bytes_transferred) {
      if (!ec)
      {
        std::string data(self->read_buffer_.data(), bytes_transferred);
        if (self->receive_callback_) self->receive_callback_(data);
        self->start_async_read();  // 继续监听
      }
      else if (ec != asio::error::operation_aborted)
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
