#include "serial_port_session.h"

SerialPortSession::SerialPortSession() :
  work_guard_(asio::make_work_guard(io_)), strand_(asio::make_strand(io_)), serial_(io_), running_(false)
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

      self->running_ = true;
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
  if (!running_) return;

  running_ = false;

  auto self = shared_from_this();

  // 保证串口关闭任务完成再 stop io
  std::promise<void> close_done;
  auto close_future = close_done.get_future();

  asio::post(strand_, [self, &close_done]() {
    if (self->serial_.is_open())
    {
      asio::error_code ec;
      self->serial_.cancel(ec);
      self->serial_.close(ec);
      self->report_info("Serial port closed.");
    }
    close_done.set_value();
  });

  close_future.wait();  // 等待关闭完成

  io_.stop();  // 终止 io_context::run()
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
        self->start_async_read();  // 持续监听
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
