add_library(trawler-services-tcp-common INTERFACE)

target_link_libraries(trawler-services-tcp-common
  INTERFACE
    trawler-services-base
    trawler-logging
    rxcpp
)

target_include_directories(trawler-services-tcp-common
  INTERFACE
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

