#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <trawler/services/http-server/http-server.hpp>

SCENARIO("dummy http-server")
{
  using namespace trawler;
  Logger::set_log_level(Logger::ELogLevel::DEBUG);
  auto context = make_service_context( );
  create_http_server(context, "0.0.0.0", 5001, { "my-http-server" })
    .filter([](const auto& s) { return s.get_status( ) == ServicePacket::EStatus::DATA_TRANSMISSION; })
    .as_blocking( )
    .subscribe([](auto s) {
      std::cout << "PAYLOAD: " << s.template get_payload_as<std::string>( ) << "\n";
      s.reply("hello world");
    });
}
