#include "overloaded.hpp"
#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

using services_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;
using pipelines_t = std::vector<configuration_t::pipeline_t>;
using subscriptions_t = std::vector<rxcpp::subscription>;

auto
make_endpoint_visitor(const services_t& services, subscriptions_t& subscriptions, const Logger& logger)
{
  return [&](const config::endpoint_pipeline_t& endpoint) {
    logger.info("Creating endpoint " + endpoint.name);
    auto predicate = [&](const auto& service) { return service.first == endpoint.source; };
    auto service = std::find_if(begin(services), end(services), std::move(predicate));

    if (service != cend(services)) {
      auto [service_name, service_obj] = *service;
      auto event = endpoint.event;
      auto filter = [event](const ServicePacket& sp) {
        if (event == "connected") {
          return sp.get_status( ) == ServicePacket::EStatus::CONNECTED;
        }

        if (event == "data") {
          return sp.get_status( ) == ServicePacket::EStatus::DATA_TRANSMISSION;
        }

        if (event == "disconnected") {
          return sp.get_status( ) == ServicePacket::EStatus::DISCONNECTED;
        }

        return true;
      };

      auto endpoint_obj = create_endpoint(endpoint.data, { endpoint.name });

      auto subscription = service_obj.filter(filter).subscribe(std::move(endpoint_obj));
      subscriptions.push_back(std::move(subscription));
    }
  };
}

std::vector<rxcpp::subscription>
spawn_pipelines(const std::shared_ptr<class ServiceContext>& context,
                const services_t& services,
                const pipelines_t& pipelines,
                const Logger& logger)
{
  logger.debug("Spawning pipelines");
  auto subscriptions = subscriptions_t{};

  auto endpoint_visitor = make_endpoint_visitor(services, subscriptions, logger);

  auto visitor = overloaded{ endpoint_visitor, [](auto) {} };
  for (const auto& pipe : pipelines) {
    std::visit(visitor, pipe);
  }

  return subscriptions;
}
}
