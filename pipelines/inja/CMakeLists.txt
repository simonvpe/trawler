add_library(trawler-pipelines-inja
  STATIC
    src/inja.cpp
)

target_link_libraries(trawler-pipelines-inja
  PRIVATE
    nlohmann_json # For some reason inja segfaults without this line here
    inja
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

set_target_properties(trawler-pipelines-inja PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-pipelines-inja)
