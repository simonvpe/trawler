trawler_add_test(
  TEST
    trawler-services-websocket
  SOURCES
    test.cpp
  LIBS
    trawler-services-websocket-server
    trawler-services-websocket-client
    trawler-logging
    doctest)
