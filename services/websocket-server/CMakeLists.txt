add_library(trawler-services-websocket-server
  STATIC
    src/websocket-server.cpp
)

target_link_libraries(trawler-services-websocket-server
  PUBLIC
    trawler-services-base
    rxcpp
  PRIVATE
    trawler-services-websocket-common
    trawler-services-tcp-common
#    OpenSSL::SSL
)

target_include_directories(trawler-services-websocket-server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-services-websocket-server PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-services-websocket-server)
