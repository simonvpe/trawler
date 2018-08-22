#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

std::function<void(const ServicePacket&)>
create_endpoint(const Logger& logger)
{
  return [=](const ServicePacket& sp) {
    auto payload = sp.template get_payload_as<std::string>( );
    logger.debug("Payload is " + payload);
    if (payload == "\n" || payload == "") {
      logger.debug("No payload to reply with");
    } else {
      logger.debug("Replying with " + payload);
      sp.reply(std::move(payload));
    }
  };
}
}
