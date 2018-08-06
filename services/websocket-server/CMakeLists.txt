add_library(trawler-services-websocket-server
  STATIC
    src/websocket-server.cpp
)

target_link_libraries(trawler-services-websocket-server
  PUBLIC
    trawler-services-base
    trawler-services-util
    trawler-services-websocket-common
    rxcpp
#    OpenSSL::SSL
)

target_include_directories(trawler-services-websocket-server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-services-websocket-server)
