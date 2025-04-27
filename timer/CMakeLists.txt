add_executable(timer_sync timer_sync.cpp)
target_link_libraries(timer_sync PRIVATE asio)

add_executable(timer_async timer_async.cpp)
target_link_libraries(timer_async PRIVATE asio)


add_executable(timer3 timer3.cpp)
target_link_libraries(timer3 PRIVATE asio)

add_executable(timer4 timer4.cpp)
target_link_libraries(timer4 PRIVATE asio)

add_executable(timer5 timer5.cpp)
target_link_libraries(timer5 PRIVATE asio)