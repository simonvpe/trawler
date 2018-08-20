add_library(trawler-services-base INTERFACE)

target_link_libraries(trawler-services-base
  INTERFACE
    rxcpp
    Threads::Threads
    Boost::system
)

target_include_directories(trawler-services-base
  INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
