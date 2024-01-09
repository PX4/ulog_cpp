cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(ulog_cpp LANGUAGES CXX)

# Determine if ulog_cpp is built as a subproject (using add_subdirectory) or if it is the main project.
set(MAIN_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(ulog_cpp)

if(MAIN_PROJECT)
	add_compile_options(
		# always enable debug symbols (for stack trace)
		-g

		# Warnings
		-Wall
		-Wextra
		-Werror

		# disabled warnings
		-Wno-missing-field-initializers
		-Wno-unused-parameter
		-Wno-float-equal # TODO: this is required for cli11 2.3.2
	)

	# compiler specific flags
	if (("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang") OR ("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang"))
		add_compile_options(
			-fcolor-diagnostics # force color for clang (needed for clang + ccache)
			-fdiagnostics-absolute-paths # force absolute paths

			-Qunused-arguments

			-Wno-unknown-warning-option
			-Wno-unused-const-variable
		)
	elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		add_compile_options(-fdiagnostics-color=always)
	endif()

	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
	include(clang_format)

	add_subdirectory(examples)
	add_subdirectory(test)

endif()

target_include_directories(${PROJECT_NAME} INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>)

