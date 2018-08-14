#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

std::function<void(const ServicePacket&)>
create_endpoint(const std::optional<std::string>& data, const Logger& logger)
{
  return [=](const ServicePacket& sp) {
    if (data) {
      logger.debug("Replying");
      sp.reply(data.value( ));
    } else {
      logger.debug("Endpoint hit, not replying");
    }
  };
}
}
