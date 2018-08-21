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
    const auto out = [tmplate, logger, payload = x.template get_payload_as<json>( )] {
      logger.debug("Received " + payload.dump( ));
      if (payload.type( ) != json::value_t::null) {
        logger.debug("Rendering");
        return inja::render(tmplate, json{ { "payload", payload } });
      }
      logger.debug("Returning empty string");
      return std::string{};
    }( );
    logger.debug(out);
    return x.with_payload(out);
  };
}
}
