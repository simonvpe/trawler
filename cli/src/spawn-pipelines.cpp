#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>

namespace trawler {

std::vector<rxcpp::subscription>
spawn_pipelines(const std::shared_ptr<class ServiceContext>& context,
                const std::vector<configuration_t::pipeline_t>& pipelines,
                const Logger& logger)
{
  logger.debug("Spawning endpoints");
  auto subscriptions = std::vector<rxcpp::subscription>{};

  return subscriptions;
}
}
