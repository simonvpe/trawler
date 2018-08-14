#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

std::function<void(const ServicePacket&)>
create_endpoint(const std::string& data, const Logger& logger)
{
  return [=](const ServicePacket& sp) {
    logger.debug("Replying");
    sp.reply(data);
  };
}
}
