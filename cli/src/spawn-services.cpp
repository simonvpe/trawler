#include "overloaded.hpp"
#include <trawler/cli/spawn-services.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>

namespace trawler {

using services_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;

services_t
spawn_services(const std::shared_ptr<ServiceContext>& context,
               const std::vector<configuration_t::service_t>& services,
               const Logger& logger)
{
  logger.debug("Spawning services");
  auto offspring = services_t{};

  auto visit_websocket_client = [&](const config::websocket_client_service_t& service) {
    if (service.ssl) {
      logger.info("Creating websocket ssl client [" + service.name + "]");
      auto client = create_websocket_client_ssl(context, service.host, service.port, service.target, { service.name });
      offspring.push_back({ service.name, std::move(client) });
    } else {
      logger.info("Creating websocket client [" + service.name + "]");
      auto client = create_websocket_client(context, service.host, service.port, service.target, { service.name });
      offspring.push_back({ service.name, std::move(client) });
    }
  };

  auto visitor = overloaded{ visit_websocket_client, [](auto) {} };
  for (const auto& svc : services) {
    std::visit(visitor, svc);
  }
  return offspring;
}
}
