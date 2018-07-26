#define DOCTEST_CONFIG_COLORS_ANSI
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>

#include <websocket-server.hpp>
#include <websocket-client.hpp>

using namespace trawler;

SCENARIO("start a server") {
  GIVEN("a websocket server") {
    auto server = websocket_server(5000);

    WHEN("a client connects") {
      auto client = websocket_client("ws://localhost:5000");

      THEN("the the server should react") {
	
      }
    }

  }
}
