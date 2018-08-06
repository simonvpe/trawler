//#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
//#include <doctest.h>
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>
#include <trawler/services/websocket-server/websocket-server.hpp>

//#include <websocket-ssl-client.hpp>

using namespace trawler;
using namespace std::chrono_literals;

void
rethrow_exception(const std::exception_ptr& eptr)
{
  try {
    if (eptr) {
      std::rethrow_exception(eptr);
    }
  } catch (const std::exception& e) {
    std::cout << "Caught exception \"" << e.what( ) << "\"\n";
  }
}

int
main( )
{
  using namespace std::chrono_literals;

  Logger::set_log_level(Logger::ELogLevel::DEBUG);

  {
    auto context = make_service_context(4, 4);
    {
      auto srv = create_websocket_server(context, "0.0.0.0", 5000);

      auto on_next = [](ServicePacket data) { std::cout << "DATA: " << data.get_payload( ) << '\n'; };

      auto on_complete = []( ) { std::cout << "COMPLETED\n"; };

      auto filter_data = [](auto packet) { return packet.get_status( ) == ServicePacket::EStatus::DATA_TRANSMISSION; };

      auto send_reply = [](const std::string& rep) {
        return [rep](ServicePacket packet) {
		 std::cout << "Replying with " << rep << '\n';
		 packet.call_on_reply(std::move(rep));
	       };
      };

      auto srvsub = srv.observe_on(rxcpp::observe_on_new_thread( ))
                      .filter(filter_data)
                      .subscribe(send_reply("PONG"), rethrow_exception, on_complete);

      /*
      auto cli2 = create_websocket_client(context, "localhost", 5000, { "client-2" })
                    .tap(send_reply("PING"))
                    .filter(filter_data);

      int i = 0;
      auto thread = rxcpp::observe_on_event_loop( );
      rxcpp::observable<>::from(cli1.as_dynamic( ), cli2)
                      .merge()
                      .take(10)
                      .as_blocking( )
                      .subscribe([&](auto s) { std::cout << "Recv " << ++i << ": " << s.get_payload( ) << '\n'; });
      */

      // .take(10) causes create_websocket_client_impl to leak!
      int i = 0;

      create_websocket_client(context, "localhost", 5000, { "client-1" })
        .observe_on(rxcpp::observe_on_new_thread( ))
        .tap(send_reply("PING"))
        .filter(filter_data)
        .take(10)
        .as_blocking( )
        .subscribe([&](auto s) { std::cout << "Recv " << ++i << ": " << s.get_payload( ) << '\n'; });

    }
  }
  return 0;
}
