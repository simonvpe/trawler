#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>
#include <trawler/services/websocket-server/websocket-server.hpp>

//#include <websocket-ssl-client.hpp>

using namespace trawler;

SCENARIO("Websocket server and client") {
  Logger::set_log_level(Logger::ELogLevel::DEBUG);

  {
    auto context = make_service_context(4, 4);

    auto filter_data = [](auto packet) { return packet.get_status( ) == ServicePacket::EStatus::DATA_TRANSMISSION; };

    auto send_reply = [](const std::string& rep) {
      return [rep](ServicePacket packet) { packet.call_on_reply(std::move(rep)); };
    };

    create_websocket_server(context, "0.0.0.0", 5000)
      .observe_on(rxcpp::observe_on_new_thread( ))
      .filter(filter_data)
      .subscribe(send_reply("PONG"));

    std::vector<std::string> result{};

    create_websocket_client(context, "localhost", 5000, { "client-1" })
      .observe_on(rxcpp::observe_on_new_thread( ))
      .tap(send_reply("PING"))
      .filter(filter_data)
      .take(10)
      .reduce(std::vector<std::string>{},
              [](auto acc, auto s) {
		CHECK( s.get_payload() == "PONG" );
                acc.push_back(s.get_payload( ));
                return acc;
              })
      .as_blocking( )
      .subscribe([&](auto r) { result = r; });
    
    CHECK(result.size() == 10);
  }
}
