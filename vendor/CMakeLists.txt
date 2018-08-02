set(DOCTEST_WITH_TESTS OFF CACHE BOOL "Disable building doctest tests")
add_subdirectory(doctest)

# RxCpp isn't well behaved, we provide our own wrapper library
add_library(rxcpp INTERFACE)
target_include_directories(rxcpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/RxCpp/Rx/v2/src)

# Rapidjson isn't well behaved either, we provide our own
# wrapper library again
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include)

add_subdirectory(fmt)
set_target_properties(
    fmt
    PROPERTIES
         CXX_CLANG_TIDY ""
)

set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "Disable building spdlog examples")
set(SPDLOG_BUILD_TESTING OFF CACHE BOOL "Disable building spdlog tests")
add_subdirectory(spdlog)
