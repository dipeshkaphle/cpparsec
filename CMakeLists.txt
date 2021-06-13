cmake_minimum_required(VERSION "3.19.2")

project(parser)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wpedantic -Wextra -fsanitize=address -O3")

set(CMAKE_CXX_STANDARD 20)

include_directories("${PROJECT_SOURCE_DIR}/includes")
if(BUILD_PROJECT STREQUAL "tests")
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  add_executable(tests tests.cpp)
  target_link_libraries(tests ${CONAN_LIBS})
else()
  add_subdirectory(examples)
  add_executable(main main.cpp)
  add_executable(tests tests.cpp)
  target_link_libraries(tests ${CONAN_LIBS})
endif()
