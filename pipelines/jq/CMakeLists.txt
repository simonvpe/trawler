add_library(trawler-pipelines-jq
  STATIC
    src/jq.cpp
)

target_link_libraries(trawler-pipelines-jq
  PRIVATE
    nlohmann_json
  PUBLIC
    jq
    trawler-services-base
    trawler-logging
    rxcpp
)

target_include_directories(trawler-pipelines-jq
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-pipelines-jq PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-pipelines-jq)
