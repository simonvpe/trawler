#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <rxcpp/rx.hpp>
#include <trawler/pipelines/buffer/buffer.hpp>

using namespace trawler;
using namespace std::chrono_literals;

SCENARIO("dummy buffer")
{
  std::vector<std::string> result;
  auto subscriber = [&](auto v) { result.push_back(v.template get_payload_as<std::string>( )); };
  auto as_service_packet = [](auto s) {
    return ServicePacket{ ServicePacket::EStatus::DATA_TRANSMISSION, { std::to_string(s) } };
  };
  auto source = rxcpp::observable<>::interval(std::chrono::steady_clock::now( ), 1ms).map(as_service_packet);
  auto trigger = rxcpp::observable<>::interval(std::chrono::steady_clock::now( ), 10ms).map(as_service_packet);
  auto sub = create_buffer_pipeline(trigger, source)
               .take(3)
               .subscribe_on(rxcpp::observe_on_new_thread( ))
               .subscribe(subscriber);
  std::this_thread::sleep_for(100ms);
  CHECK(result.size( ) == 3);
  sub.unsubscribe( );
}
