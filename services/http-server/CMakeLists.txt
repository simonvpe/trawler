add_library(trawler-services-http-server
  STATIC
    src/http-server.cpp
)

target_link_libraries(trawler-services-http-server
  PUBLIC
    trawler-services-base
    trawler-services-tcp-common
    trawler-logging
    nlohmann_json
    rxcpp
)

target_include_directories(trawler-services-http-server
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-services-http-server PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-services-http-server)
