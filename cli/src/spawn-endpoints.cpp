#include "overloaded.hpp"
#include <trawler/cli/spawn-endpoints.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

std::vector<rxcpp::subscription>
spawn_endpoints(const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& services,
                const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& pipelines,
                const std::vector<configuration_t::endpoint_t>& endpoint_config,
                const Logger& logger)
{
  logger.debug("Spawning endpoints");
  auto subscriptions = std::vector<rxcpp::subscription>{};

  for (const auto& endpoint_name : endpoint_config) {
    const auto predicate = [endpoint_name](const auto& x) { return x.first == endpoint_name; };
    auto endpoint = create_endpoint({ endpoint_name + ".endpoint" });

    auto service = std::find_if(cbegin(services), cend(services), std::move(predicate));
    if (service != cend(services)) {
      auto subscription = service->second.subscribe(std::move(endpoint));
      subscriptions.push_back(std::move(subscription));
      continue;
    }

    auto pipeline = std::find_if(cbegin(pipelines), cend(pipelines), std::move(predicate));
    if (pipeline != cend(pipelines)) {
      auto subscription = pipeline->second.subscribe(std::move(endpoint));
      subscriptions.push_back(std::move(subscription));
    }
  }

  return subscriptions;
}
}
