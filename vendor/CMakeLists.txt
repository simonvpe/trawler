set(DOCTEST_WITH_TESTS OFF CACHE BOOL "Disable building doctest tests")
add_subdirectory(doctest)

# RxCpp
# RxCpp isn't well behaved, we provide our own wrapper library
add_library(rxcpp INTERFACE)
target_include_directories(rxcpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/RxCpp/Rx/v2/src)

# Rapidjson
# Rapidjson isn't well behaved either, we provide our own
# wrapper library again
add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/rapidjson/include)

# Spdlog
set(SPDLOG_BUILD_EXAMPLES OFF CACHE BOOL "Disable building spdlog examples")
set(SPDLOG_BUILD_TESTING OFF CACHE BOOL "Disable building spdlog tests")
add_subdirectory(spdlog)

# yaml-cpp
set(YAML_CPP_BUILD_TESTS OFF)
set(YAML_CPP_BUILD_TOOLS OFF)
set(YAML_CPP_BUILD_CONTRIB OFF)
add_subdirectory(yaml-cpp)
set_target_properties(yaml-cpp
  PROPERTIES
    CXX_CLANG_TIDY ""
)

# inja
set(BUILD_UNIT_TESTS OFF)
add_subdirectory(inja)
