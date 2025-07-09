#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>

#include <cstring>
#endif

std::vector<std::string> list_serial_ports()
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
