#include "serial_port_session.h"

#include <future>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>

#include <cstring>
#endif

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

std::vector<std::string> SerialPortSession::list_serial_ports()
{
  std::vector<std::string> ports;

#if defined(_WIN32)
  // 遍历 COM1 ~ COM256
  for (int i = 1; i <= 256; ++i)
  {
    std::string port_name = "COM" + std::to_string(i);
    std::string full_name = "\\\\.\\" + port_name;

    // 只检查存在性，避免权限和独占问题
    HANDLE h = CreateFileA(full_name.c_str(), 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE)
    {
      ports.push_back(port_name);
      CloseHandle(h);
    }
    else
    {
      DWORD err = GetLastError();
      if (err == ERROR_ACCESS_DENIED || err == ERROR_GEN_FAILURE)
      {
        ports.push_back(port_name);  // 存在但已被占用，也算有效
      }
    }
  }

#else
  const std::string dev_path = "/dev/";
  const std::vector<std::string> prefixes = {
    "ttyS", "ttyUSB", "ttyACM", "ttyAMA", "rfcomm", "tty.", "cu."  // 支持 macOS 和树莓派
  };

  DIR* dir = opendir(dev_path.c_str());
  if (dir == nullptr)
  {
    return ports;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    std::string name = entry->d_name;

    for (const auto& prefix : prefixes)
    {
      if (name.compare(0, prefix.size(), prefix) == 0)
      {
        std::string full_path = dev_path + name;

        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISCHR(st.st_mode))
        {
          ports.push_back(full_path);
        }

        break;
      }
    }
  }

  closedir(dir);
#endif

  return ports;
}

bool SerialPortSession::start()
{
  if (running_) return true;
  if (!open())  // 打开串口
  {
    return false;
  }
  io_thread_ = std::thread([this]() { io_.run(); });  // 启动线程
  running_ = true;
  return true;
}

bool SerialPortSession::open()
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
    report_error("Failed to open serial port: " + port_name_ + ", " + std::string(e.what()));
    return false;
  }
  return true;
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
  if (!running_)
  {
    report_error("Send error: Serial port is not open.");
    return;
  };

  auto self = shared_from_this();
  auto data_ptr = std::make_shared<std::string>(std::move(data));  // 使用 shared_ptr 确保数据在异步操作期间有效

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
  if (error_callback_) error_callback_("[SerialPort INFO] " + msg);
}

void SerialPortSession::report_warn(const std::string& msg)
{
  if (error_callback_) error_callback_("[SerialPort WARN] " + msg);
}

void SerialPortSession::report_error(const std::string& msg)
{
  if (error_callback_) error_callback_("[SerialPort ERROR] " + msg);
}
