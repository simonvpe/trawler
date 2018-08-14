add_library(trawler-pipelines-endpoint
  STATIC
    src/endpoint.cpp
)

target_link_libraries(trawler-pipelines-endpoint
  PUBLIC
    trawler-services-base
    trawler-logging
    rxcpp
#    OpenSSL::SSL
)

target_include_directories(trawler-pipelines-endpoint
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-pipelines-endpoint)
