add_library(trawler-common INTERFACE)
target_include_directories(trawler-common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(trawler-common INTERFACE cxx_std_14)
