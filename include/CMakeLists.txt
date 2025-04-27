add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

# POSIX 系统下使用 pthread 库
if(UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(asio INTERFACE Threads::Threads)
endif()