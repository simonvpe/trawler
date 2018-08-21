#include <inja.hpp>
#include <iostream>
#include <nlohmann/json.hpp>
#include <trawler/pipelines/inja/inja.hpp>

namespace trawler {

using json = nlohmann::json;

std::function<ServicePacket(ServicePacket)>
create_inja_pipeline(const std::string& tmplate, const Logger& logger)
{
  return [=](const auto& x) {
    const auto payload = x.template get_payload_as<json>( );
    logger.debug("Payload " + payload.dump( ));
    auto result = inja::render(tmplate, payload);
    return x.with_payload(result);
  };
}
}
