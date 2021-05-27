cmake_minimum_required(VERSION "3.19.2")

project(parser)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wpedantic -Wextra -Werror -fsanitize=address -O3")

set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

include_directories("${PROJECT_SOURCE_DIR}/includes")

add_executable(main main.cpp)
add_executable(tests tests.cpp)
target_link_libraries(tests ${CONAN_LIBS})
