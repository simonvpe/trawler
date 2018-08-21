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

constexpr auto filter_data = [](auto packet) {
  return packet.get_status( ) == ServicePacket::EStatus::DATA_TRANSMISSION;
};

constexpr auto send_reply = [](const std::string& rep) {
  return [rep](ServicePacket packet) { packet.reply(std::move(rep)); };
};

SCENARIO("Websocket server and client")
{
  Logger::set_log_level(Logger::ELogLevel::DEBUG);

  auto context = make_service_context(1, 1);

  create_websocket_server(context, "0.0.0.0", 5000)
    .observe_on(rxcpp::observe_on_new_thread( ))
    .filter(filter_data)
    .subscribe(send_reply("PONG"));

  std::vector<std::string> result{};

  auto cli1 = create_websocket_client(context, "localhost", 5000, "/", { "client-1" })
                .observe_on(rxcpp::observe_on_new_thread( ))
                .tap(send_reply("PING"))
                .filter(filter_data)
                .take(10);

  auto cli2 = create_websocket_client(context, "localhost", 5000, "/", { "client-2" })
                .observe_on(rxcpp::observe_on_new_thread( ))
                .tap(send_reply("PING"))
                .filter(filter_data)
                .take(10);

  cli1.merge(cli2)
    .reduce(std::vector<std::string>{},
            [](auto acc, auto s) {
              const auto payload = s.template get_payload_as<std::string>( );
              CHECK(payload == "PONG");
              acc.push_back(payload);
              return acc;
            })
    .as_blocking( )
    .subscribe([&](auto r) { result = r; });

  CHECK(result.size( ) == 20);
}

SCENARIO("Websocket ssl client")
{
  auto context = make_service_context(1, 1);

  create_websocket_client_ssl(context, "ws.blockchain.info", 443, "/inv", { "blockchain-client" })
    .tap(send_reply(R"/({"op":"ping"})/"))
    .filter(filter_data)
    .take(1)
    .as_blocking( )
    .subscribe([](auto p) { CHECK(p.template get_payload_as<std::string>( ) == R"/({"op":"pong"})/"); });
}
