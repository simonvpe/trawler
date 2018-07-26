add_library(trawler-sources INTERFACE)
target_link_libraries(trawler-sources
  INTERFACE
  trawler-common
  rxcpp
  Threads::Threads
  Boost::boost
  Boost::system)
target_include_directories(trawler-sources INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(trawler-sources INTERFACE cxx_std_14)
