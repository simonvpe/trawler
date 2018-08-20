#include <trawler/services/http-server/http-server.hpp>

namespace trawler {

rxcpp::observable<class ServicePacket>
create_http_server(const Logger& logger)
{
  return rxcpp::observable<>::create<ServicePacket>([](auto subscriber) {});
}
}
