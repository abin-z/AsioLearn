add_executable(timer_sync timer_sync.cpp)
target_link_libraries(timer_sync PRIVATE asio)

add_executable(timer_async timer_async.cpp)
target_link_libraries(timer_async PRIVATE asio)