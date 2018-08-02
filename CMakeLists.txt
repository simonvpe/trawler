cmake_minimum_required(VERSION 3.11)
project(trawler)

option(TRAWLER_ENABLE_SANITIZERS "Enable sanitizers" OFF)

function(trawler_add_sanitizers TARGET)
  if(${TRAWLER_ENABLE_SANITIZERS})
    set_property(TARGET ${TARGET}
      APPEND_STRING PROPERTY COMPILE_FLAGS "-fsanitize=address"
    )
    target_link_libraries(${TARGET} PUBLIC asan)
  endif(${TRAWLER_ENABLE_SANITIZERS})
endfunction()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/CMake)
find_package(Boost 1.66 COMPONENTS system REQUIRED)
find_package(jq REQUIRED) # Version 1.5 required but headers don't provide this info
find_package(Threads)
find_package(OpenSSL REQUIRED)

add_subdirectory(logging)
add_subdirectory(services)
add_subdirectory(vendor)

enable_testing()
add_subdirectory(test)

