#include <nlohmann/json.hpp>
#include <trawler/pipelines/buffer/buffer.hpp>

namespace trawler {

rxcpp::observable<ServicePacket>
create_buffer_pipeline(rxcpp::observable<ServicePacket> trigger,
                       rxcpp::observable<ServicePacket> source,
                       const Logger& logger)
{
  auto combine = [](const ServicePacket& trigger_packet, const ServicePacket& source_packet) {
    nlohmann::json payload{};
    if (std::holds_alternative<nlohmann::json>(source_packet.get_payload( ))) {
      payload["source"] = source_packet.template get_payload_as<nlohmann::json>( );
    } else {
      payload["source"] = source_packet.template get_payload_as<std::string>( );
    }

    if (std::holds_alternative<nlohmann::json>(trigger_packet.get_payload( ))) {
      payload["trigger"] = trigger_packet.template get_payload_as<nlohmann::json>( );
    } else {
      payload["trigger"] = trigger_packet.template get_payload_as<std::string>( );
    }

    return trigger_packet.with_payload({ std::move(payload) });
  };

  return trigger.with_latest_from(std::move(combine), source).as_dynamic( );
}
}
