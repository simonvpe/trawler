add_library(trawler-services-websocket-common
  STATIC
    src/make-runtime-error.cpp
)

target_link_libraries(trawler-services-websocket-common
  PUBLIC
    trawler-services-base
    trawler-logging
    rxcpp
#    OpenSSL::SSL
)

target_include_directories(trawler-services-websocket-common
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-services-websocket-common)
