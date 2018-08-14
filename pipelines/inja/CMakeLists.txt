add_library(trawler-pipelines-inja
  STATIC
    src/inja.cpp
)

target_link_libraries(trawler-pipelines-inja
  PUBLIC
    trawler-services-base
    trawler-logging
    rxcpp
)

target_include_directories(trawler-pipelines-inja
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-pipelines-inja)
