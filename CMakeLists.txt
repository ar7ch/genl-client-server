cmake_minimum_required(VERSION 3.21)
project(genl-client-server VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)  # enable GNU extensions

# Set the output directory for binaries
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# warn if ${CMAKE_INCLUDE_PATH} is not set
if(NOT DEFINED CMAKE_INCLUDE_PATH)
	message(WARNING "CMAKE_INCLUDE_PATH is not set")
endif()

# include directories
include_directories(${CMAKE_INCLUDE_PATH})

# Source files
set(SOURCES
	src/main.cpp
)

# add executable
add_executable(${PROJECT_NAME} ${SOURCES})

include_directories(${CMAKE_INCLUDE_PATH}
	${CMAKE_SOURCE_DIR}/src
	libs/cli11/include
	libs/spdlog/include
	libs/include
)


add_subdirectory(libs/cli11)
add_subdirectory(libs/spdlog)
add_subdirectory(libnl++)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
target_compile_options(${PROJECT_NAME} PRIVATE -fpermissive -Wno-unused-parameter -Wfatal-errors)
target_link_libraries(${PROJECT_NAME} PRIVATE libnl++ CLI11::CLI11)

# allow the user to specify the installation directory via cmake variable
set(INSTALL_DIR_DEFAULT "${CMAKE_BINARY_DIR}/install" CACHE PATH "Installation directory")

# install the target to the specified directory
install(TARGETS ${PROJECT_NAME} DESTINATION "${INSTALL_DIR_DEFAULT}")
