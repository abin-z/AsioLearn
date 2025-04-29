add_executable(serial_port serial_port.cpp)
target_link_libraries(serial_port PRIVATE asio)

add_executable(list_serial_port list_serial_port.cpp)
target_link_libraries(list_serial_port PRIVATE asio)

