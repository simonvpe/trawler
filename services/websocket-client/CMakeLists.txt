add_library(trawler-services-websocket-client
  STATIC
    src/websocket-client.cpp
    src/websocket-client-ssl.cpp
)

target_link_libraries(trawler-services-websocket-client
  PUBLIC
    trawler-services-base
    rxcpp
    OpenSSL::SSL
  PRIVATE
    trawler-services-websocket-common
)

target_include_directories(trawler-services-websocket-client
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-services-websocket-client PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-services-websocket-client)
