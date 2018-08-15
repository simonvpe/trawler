# Library
add_library(trawler-cli
  STATIC
    src/parse-options.cpp
    src/parse-configuration.cpp
    src/spawn-services.cpp
    src/spawn-pipelines.cpp
    src/spawn-endpoints.cpp
)

target_link_libraries(trawler-cli
  PRIVATE
    yaml-cpp
    trawler-services-websocket-client
    trawler-services-websocket-server
    trawler-pipelines-endpoint
    trawler-pipelines-inja
    trawler-pipelines-jq    
    trawler-logging
  PUBLIC
    Boost::program_options
    trawler-services-base
)

target_include_directories(trawler-cli
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-cli)

set_target_properties(trawler-cli PROPERTIES CXX_STANDARD 17)

# Executable
add_executable(trawler
  src/trawler.cpp
)

target_link_libraries(trawler
  PRIVATE
    trawler-cli
    trawler-logging
)

trawler_add_sanitizers(trawler)

set_target_properties(trawler PROPERTIES CXX_STANDARD 17)
