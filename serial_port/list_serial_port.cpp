#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>

std::vector<std::string> list_serial_ports()
{
  std::vector<std::string> ports;
  for (int i = 1; i <= 256; ++i)
  {
    std::string port_name = "COM" + std::to_string(i);
    std::string full_name = "\\\\.\\" + port_name;

    HANDLE h = CreateFileA(full_name.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h != INVALID_HANDLE_VALUE)
    {
      ports.push_back(port_name);
      CloseHandle(h);
    }
  }
  return ports;
}

#else  // Linux or Unix-like
#include <dirent.h>
#include <sys/stat.h>

std::vector<std::string> list_serial_ports()
{
  std::vector<std::string> ports;
  const std::string dev_path = "/dev/";
  const std::vector<std::string> prefixes = {"ttyS", "ttyUSB", "ttyACM"};

  DIR* dir = opendir(dev_path.c_str());
  if (!dir) return ports;

  struct dirent* entry;
  while ((entry = readdir(dir)) != nullptr)
  {
    std::string name(entry->d_name);
    for (const auto& prefix : prefixes)
    {
      if (name.find(prefix) == 0)
      {
        ports.push_back(dev_path + name);
        break;
      }
    }
  }

  closedir(dir);
  return ports;
}
#endif

int main()
{
  auto ports = list_serial_ports();
  if (ports.empty())
  {
    std::cout << "No serial ports found.\n";
  }
  else
  {
    std::cout << "Available serial ports:\n";
    for (const auto& port : ports)
    {
      std::cout << "  " << port << "\n";
    }
  }
  return 0;
}
