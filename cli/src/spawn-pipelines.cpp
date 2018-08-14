#include "overloaded.hpp"
#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

using services_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;
using pipelines_t = std::vector<configuration_t::pipeline_t>;
using subscriptions_t = std::vector<rxcpp::subscription>;

std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>
spawn_pipelines(const std::shared_ptr<class ServiceContext>& context,
                const services_t& services,
                const pipelines_t& pipeline_config,
                const Logger& logger)
{
  logger.debug("Spawning pipelines");
  auto subscriptions = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>{};

  auto visitor = overloaded{ [](auto) {} };
  for (const auto& pipe : pipeline_config) {
    std::visit(visitor, pipe);
  }

  return subscriptions;
}
}
