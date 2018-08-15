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
    const auto out = [tmplate, in = x.get_payload( )] {
      if (in.size( ) > 0) {
        return inja::render(tmplate, json{ { "payload", json::parse(in) } });
      } else {
        return std::string{};
      }
    }( );
    logger.debug(out);
    return x.with_payload(out);
  };
}
}
