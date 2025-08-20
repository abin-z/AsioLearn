add_executable(tcp_client tcp_client.cpp)
target_link_libraries(tcp_client PRIVATE asio)

add_executable(tcp_server tcp_server.cpp)
target_link_libraries(tcp_server PRIVATE asio)

add_executable(tcp_server_async tcp_server_async.cpp)
target_link_libraries(tcp_server_async PRIVATE asio)

add_executable(tcp_server_async2 tcp_server_async2.cpp)
target_link_libraries(tcp_server_async2 PRIVATE asio)

add_executable(tcp_client2 tcp_client2.cpp)
target_link_libraries(tcp_client2 PRIVATE network)