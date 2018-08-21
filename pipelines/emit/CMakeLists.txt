add_library(trawler-pipelines-emit
  STATIC
    src/emit.cpp
)

target_link_libraries(trawler-pipelines-emit
  PUBLIC
    trawler-services-base
    trawler-logging
    rxcpp
)

target_include_directories(trawler-pipelines-emit
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-pipelines-emit PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-pipelines-emit)
