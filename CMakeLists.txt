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
	include(CMakePackageConfigHelpers)

	write_basic_package_version_file(
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
		VERSION 1.0.0
		COMPATIBILITY AnyNewerVersion
	)

	configure_package_config_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
		INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
	)

	install(FILES
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
		"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
		DESTINATION lib/cmake/${PROJECT_NAME}
	)


	add_compile_options(
		# Warnings
		-Wall
		-Wextra
		-Werror

		# disabled warnings
		-Wno-missing-field-initializers
		-Wno-unused-parameter
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

