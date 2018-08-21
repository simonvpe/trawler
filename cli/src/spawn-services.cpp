#include "overloaded.hpp"
#include <trawler/cli/spawn-services.hpp>
#include <trawler/services/http-server/http-server.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>

namespace trawler {

using sources_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;

auto
make_websocket_client_visitor(const std::shared_ptr<ServiceContext>& context, sources_t& sources, const Logger& logger)
{
  return [&](const config::websocket_client_service_t& service) {
    if (service.ssl) {
      logger.info("Creating websocket ssl client [" + service.name + "]");
      auto client = create_websocket_client_ssl(context, service.host, service.port, service.target, { service.name })
                      .publish( )
                      .ref_count( );
      sources.emplace_back(service.name, std::move(client));
    } else {
      logger.info("Creating websocket client [" + service.name + "]");
      auto client = create_websocket_client(context, service.host, service.port, service.target, { service.name })
                      .publish( )
                      .ref_count( );
      sources.emplace_back(service.name, std::move(client));
    }
  };
}

auto
make_http_server_visitor(const std::shared_ptr<ServiceContext>& context, sources_t& sources, const Logger& logger)
{
  return [&](const config::http_server_service_t& service) {
    logger.info("Creating http server [" + service.name + "]");
    auto server = create_http_server(context, service.host, service.port, { service.name }).publish( ).ref_count( );
    sources.emplace_back(service.name, std::move(server));
  };
}

sources_t
spawn_services(const std::shared_ptr<ServiceContext>& context,
               const std::vector<configuration_t::service_t>& services,
               const Logger& logger)
{
  logger.debug("Spawning services");
  auto sources = sources_t{};

  auto websocket_client_visitor = make_websocket_client_visitor(context, sources, logger);
  auto http_server_visitor = make_http_server_visitor(context, sources, logger);

  auto visitor = overloaded{ std::move(websocket_client_visitor), std::move(http_server_visitor) };
  for (const auto& svc : services) {
    std::visit(visitor, svc);
  }
  return sources;
}
}
