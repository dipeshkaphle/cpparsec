cmake_minimum_required(VERSION "3.19.2")

project(parser)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-Wall -Wpedantic -Wextra -fsanitize=address -O3")

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(includes)
add_library(${PROJECT_NAME} INTERFACE)
target_link_libraries(${PROJECT_NAME} INTERFACE Parser)
if(BUILD_PROJECT STREQUAL "tests")
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  add_executable(tests tests.cpp)
  target_link_libraries(tests ${CONAN_LIBS} Parser)
elseif(BUILD_PROJECT STREQUAL "binary")
  add_subdirectory(examples)
  add_executable(main main.cpp)
  target_link_libraries(main Parser)
elseif(BUILD_PROJECT STREQUAL "all")
  include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
  conan_basic_setup()
  add_executable(tests tests.cpp)
  target_link_libraries(tests ${CONAN_LIBS} Parser)
  add_subdirectory(examples)
  add_executable(main main.cpp)
  target_link_libraries(main Parser)
else()

endif()
