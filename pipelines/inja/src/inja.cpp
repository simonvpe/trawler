#include <iostream>
#include <trawler/pipelines/inja/inja.hpp>

namespace trawler {

std::function<ServicePacket(ServicePacket)>
create_inja_pipeline(const std::string& tmplate, const Logger& logger)
{
  return [](auto x) {
    std::cout << "INJA\n";
    return x;
  };
}
}
