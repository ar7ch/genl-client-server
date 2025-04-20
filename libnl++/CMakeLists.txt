# Define the library target (STATIC, SHARED, or OBJECT; STATIC is default)
set(LIB_NAME nl++)
add_library(${LIB_NAME})
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find libnl-genl on system
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL_GENL REQUIRED libnl-3.0 libnl-genl-3.0)

# Add sources to the library
target_sources(${LIB_NAME}
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}/src/callback.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/message.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/socket.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/util.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/src/genl.cpp
	PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/include/lib${LIB_NAME}
)

# Set include directories for consumers
target_include_directories(${LIB_NAME} SYSTEM
	PUBLIC
		${LIBNL_GENL_INCLUDE_DIRS}
		${LIBNL_GENL_INCLUDE_DIRS}/libnl3
)


target_include_directories(${LIB_NAME}
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(${LIB_NAME} PRIVATE -Wall -Wfatal-errors)

target_link_directories(${LIB_NAME} PRIVATE ${LIBNL_GENL_LIBRARY_DIRS})
target_link_libraries(${LIB_NAME} PRIVATE spdlog::spdlog nl-3 nl-genl-3)
