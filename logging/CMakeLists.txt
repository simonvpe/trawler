add_library(trawler-logging
  STATIC
    src/logger.cpp
)

target_link_libraries(trawler-logging
  PUBLIC
    spdlog
)

target_include_directories(trawler-logging
  PUBLIC
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src  
)

trawler_add_sanitizers(trawler-logging)
