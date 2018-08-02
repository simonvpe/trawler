add_library(trawler-services-websocket-client
  STATIC
    src/websocket-client.cpp
)

target_link_libraries(trawler-services-websocket-client
  PUBLIC
    trawler-services-base
    trawler-services-util
    rxcpp
#    OpenSSL::SSL
)

target_include_directories(trawler-services-websocket-client
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-services-websocket-client)
