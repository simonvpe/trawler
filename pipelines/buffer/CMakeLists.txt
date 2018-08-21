add_library(trawler-pipelines-buffer
  STATIC
    src/buffer.cpp
)

target_link_libraries(trawler-pipelines-buffer
  PUBLIC
    trawler-services-base
    trawler-logging
    rxcpp
)

target_include_directories(trawler-pipelines-buffer
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(trawler-pipelines-buffer PROPERTIES CXX_STANDARD 17)

trawler_add_sanitizers(trawler-pipelines-buffer)
