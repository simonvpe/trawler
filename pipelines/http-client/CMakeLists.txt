add_library(trawler-pipelines-http-client
  STATIC
    src/http-client.cpp
)

target_link_libraries(trawler-pipelines-http-client
  PUBLIC
    trawler-services-base
    trawler-logging
    rxcpp
)

target_include_directories(trawler-pipelines-http-client
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-pipelines-http-client PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-pipelines-http-client)
