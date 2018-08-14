#include "overloaded.hpp"
#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>
#include <trawler/pipelines/inja/inja.hpp>

namespace trawler {

using services_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;
using pipelines_t = services_t;

auto
make_inja_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::inja_pipeline_t& pipe) {
    auto predicate = [name = pipe.source](const auto& svc) { return svc.first == name; };
    auto service = std::find_if(cbegin(services), cend(services), predicate);
    if (service != cend(services)) {
      logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
      auto obs = service->second.map(create_inja_pipeline(pipe.tmplate, { pipe.name })).as_dynamic( );
      pipelines.push_back({ pipe.name, std::move(obs) });
    }
  };
}

pipelines_t
spawn_pipelines(const std::shared_ptr<class ServiceContext>& context,
                const services_t& services,
                const std::vector<configuration_t::pipeline_t>& pipeline_config,
                const Logger& logger)
{
  logger.debug("Spawning pipelines");
  auto pipelines = pipelines_t{};

  auto inja_visitor = make_inja_visitor(services, pipelines, logger);

  auto visitor = overloaded{ inja_visitor, [](auto) { throw std::runtime_error("Unknown pipeline"); } };
  for (const auto& pipe : pipeline_config) {
    std::visit(visitor, pipe);
  }

  return pipelines;
}
}
