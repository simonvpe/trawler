#include "overloaded.hpp"
#include <trawler/cli/spawn-endpoints.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

std::vector<rxcpp::subscription>
spawn_endpoints(const std::shared_ptr<class ServiceContext>& context,
                const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& services,
                const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& pipelines,
                const std::vector<configuration_t::endpoint_t>& endpoint_config,
                const Logger& logger)
{
  logger.debug("Spawning endpoints");
  auto subscriptions = std::vector<rxcpp::subscription>{};

  for (const auto& endpoint : endpoint_config) {
    auto predicate = [name = endpoint.source](auto thing) { return thing.first == name; };

    auto filter = [event = endpoint.event](const ServicePacket& sp) {
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

    auto service = find_if(cbegin(services), cend(services), predicate);
    if (service != cend(services)) {
      logger.info("Creating endpoint [" + endpoint.name + "]");
      auto endpoint_obj = create_endpoint(endpoint.data, { endpoint.name });
      auto subscription = service->second.filter(filter).subscribe(std::move(endpoint_obj));
      subscriptions.push_back(std::move(subscription));
      continue;
    }

    auto pipeline = find_if(cbegin(pipelines), cend(pipelines), predicate);
    if (pipeline != cend(pipelines)) {
      logger.info("Creating endpoint [" + endpoint.name + "]");
      auto endpoint_obj = create_endpoint(endpoint.data, { endpoint.name });
      auto subscription = pipeline->second.filter(filter).subscribe(std::move(endpoint_obj));
      subscriptions.push_back(std::move(subscription));
      continue;
    }
  }
  return subscriptions;
}
}
