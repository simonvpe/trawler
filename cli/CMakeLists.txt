add_library(trawler-cli
  STATIC
    src/parse-options.cpp
)

target_link_libraries(trawler-cli
  PUBLIC
    Boost::program_options
)

target_include_directories(trawler-cli
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

trawler_add_sanitizers(trawler-cli)
