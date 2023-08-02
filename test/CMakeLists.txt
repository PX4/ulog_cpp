

include(FetchContent)
FetchContent_Declare(
	DocTest
	GIT_REPOSITORY "https://github.com/doctest/doctest.git"
	GIT_TAG "v2.4.11"
	SYSTEM
)

FetchContent_MakeAvailable(DocTest)


add_executable(tests
    main.cpp
    ulog_parsing_test.cpp
)

target_link_libraries(tests PUBLIC
    doctest::doctest
    ulog_cpp::ulog_cpp
)

add_custom_target(
	run-unit-tests
	COMMAND $<TARGET_FILE:tests>
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
	USES_TERMINAL
)
