set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

add_executable(ulog_info ulog_info.cpp)
target_link_libraries(ulog_info PUBLIC
		ulog_cpp::ulog_cpp
		)

add_executable(ulog_data ulog_data.cpp)
target_link_libraries(ulog_data PUBLIC
		ulog_cpp::ulog_cpp
		)

add_executable(ulog_writer ulog_writer.cpp)
target_link_libraries(ulog_writer PUBLIC
		ulog_cpp::ulog_cpp
		)
