cmake_minimum_required(VERSION 3.11)
project(trawler)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake)
find_package(Boost 1.67 COMPONENTS system REQUIRED)
find_package(jq REQUIRED) # Version 1.5 required but headers don't provide this info
find_package(Threads)
  
add_subdirectory(src)
add_subdirectory(vendor)

enable_testing()
add_subdirectory(test)

