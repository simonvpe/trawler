#include <trawler/pipelines/emit/emit.hpp>

namespace trawler {

std::function<ServicePacket(ServicePacket)>
create_emit_pipeline(const std::string& data, const Logger& logger)
{
  return [=](ServicePacket x) {
    logger.debug("Emitting " + data);
    return x.with_payload({ data });
  };
}
}
