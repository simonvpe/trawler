set(PROJ ${CMAKE_PROJECT_NAME})

add_executable(${PROJ}-websocket-sources-test test.cpp)
target_link_libraries(${PROJ}-websocket-sources-test trawler-sources doctest)
add_test(${PROJ}-websocket-sources-test ${PROJ}-websocket-sources-test)
