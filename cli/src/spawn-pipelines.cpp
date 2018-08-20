#include "overloaded.hpp"
#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>
#include <trawler/pipelines/inja/inja.hpp>
#include <trawler/pipelines/jq/jq.hpp>

namespace trawler {

using services_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;
using pipelines_t = services_t;

auto
find_source(const services_t& services, const pipelines_t& pipelines, const std::string& name)
{
  auto predicate = [name](const auto& x) { return x.first == name; };

  auto service = std::find_if(cbegin(services), cend(services), predicate);
  if (service != cend(services)) {
    return service->second;
  }

  auto pipeline = std::find_if(cbegin(pipelines), cend(pipelines), predicate);
  if (pipeline != cend(pipelines)) {
    return pipeline->second;
  }

  throw std::runtime_error("Failed to find service or pipeline [" + name + "]");
}

auto
make_inja_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::inja_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    auto source = find_source(services, pipelines, pipe.source);
    auto observer = source.map(create_inja_pipeline(pipe.tmplate, { pipe.name })).as_dynamic( );
    pipelines.push_back({ pipe.name, std::move(observer) });
  };
}

auto
make_jq_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::jq_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    auto source = find_source(services, pipelines, pipe.source);
    auto observer = source.flat_map(create_jq_pipeline(pipe.script, { pipe.name })).as_dynamic( );
    pipelines.push_back({ pipe.name, std::move(observer) });
  };
}

pipelines_t
spawn_pipelines(const services_t& services,
                const std::vector<configuration_t::pipeline_t>& pipeline_config,
                const Logger& logger)
{
  logger.debug("Spawning pipelines");
  auto pipelines = pipelines_t{};

  auto inja_visitor = make_inja_visitor(services, pipelines, logger);
  auto jq_visitor = make_jq_visitor(services, pipelines, logger);

  auto visitor = overloaded{ std::move(inja_visitor), std::move(jq_visitor) };
  for (const configuration_t::pipeline_t& pipe : pipeline_config) {
    std::visit(visitor, pipe);
  }

  return pipelines;
}
}
