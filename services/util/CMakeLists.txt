add_library(trawler-services-util
  INTERFACE
)

target_link_libraries(trawler-services-base
  INTERFACE
    trawler-logging
    trawler-services-base
    rxcpp
    fmt
    Threads::Threads
    Boost::boost
    Boost::system
)

target_include_directories(trawler-services-util
  INTERFACE
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
