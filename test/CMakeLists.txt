include(FetchContent)

set(SYSTEM_ARG SYSTEM)
# cmake < 3.25 fails with system attribute
if (CMAKE_VERSION VERSION_LESS "3.25")
	set(SYSTEM_ARG "")
endif ()

find_package(doctest CONFIG)

if(NOT doctest_FOUND)
	include(FetchContent)
	FetchContent_Declare(
		doctest
		GIT_REPOSITORY "https://github.com/doctest/doctest.git"
		GIT_TAG "v2.4.11"
		${SYSTEM_ARG}
	)
	FetchContent_MakeAvailable(doctest)
endif()

add_executable(tests
	main.cpp
	ulog_parsing_test.cpp
	read_api_test.cpp
)

target_link_libraries(tests PUBLIC
	doctest::doctest
	ulog_cpp::ulog_cpp
)

add_dependencies(tests ulog_cpp)

add_custom_target(
	run-unit-tests
	COMMAND $<TARGET_FILE:tests>
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	USES_TERMINAL
)

add_dependencies(run-unit-tests tests)

enable_testing()
add_test(NAME unit-tests COMMAND tests)
