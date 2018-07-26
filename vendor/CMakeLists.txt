set(DOCTEST_WITH_TESTS OFF)
SET(DOCTEST_WITH_TESTS OFF CACHE BOOL "Disable building doctest tests")
add_subdirectory(doctest)

# RxCpp isn't well behaved, we provide our own wrapper library
#add_subdirectory(RxCpp)
add_library(rxcpp INTERFACE)
target_include_directories(rxcpp INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/RxCpp/Rx/v2/src)

