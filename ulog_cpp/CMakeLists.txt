
add_library(ulog_cpp
	data_container.cpp
	messages.cpp
	reader.cpp
	writer.cpp
	simple_writer.cpp
)

add_executable(ulog_info ulog_info.cpp)
target_link_libraries(ulog_info PUBLIC
		ulog_cpp
		)

add_executable(ulog_data ulog_data.cpp)
target_link_libraries(ulog_data PUBLIC
		ulog_cpp
		)

add_executable(ulog_writer ulog_writer.cpp)
target_link_libraries(ulog_writer PUBLIC
		ulog_cpp
		)
