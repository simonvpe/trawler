#include "overloaded.hpp"
#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/pipelines/buffer/buffer.hpp>
#include <trawler/pipelines/emit/emit.hpp>
#include <trawler/pipelines/endpoint/endpoint.hpp>
#include <trawler/pipelines/http-client/http-client.hpp>
#include <trawler/pipelines/inja/inja.hpp>
#include <trawler/pipelines/jq/jq.hpp>

namespace trawler {

using services_t = std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>;
using pipelines_t = services_t;

auto
find_source(const services_t& services, const pipelines_t& pipelines, const std::string& name)
{
  const auto predicate = [name](const auto& x) { return x.first == name; };

  const auto service = std::find_if(cbegin(services), cend(services), predicate);
  if (service != cend(services)) {
    return service->second;
  }

  const auto pipeline = std::find_if(cbegin(pipelines), cend(pipelines), predicate);
  if (pipeline != cend(pipelines)) {
    return pipeline->second;
  }

  throw std::runtime_error("Failed to find service or pipeline [" + name + "]");
}

auto
make_event_filter(std::vector<ServicePacket::EStatus> accept)
{
  return [accept = std::move(accept)](const ServicePacket& service_packet) {
    return std::find(cbegin(accept), cend(accept), service_packet.get_status( )) != cend(accept);
  };
}

auto
make_inja_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::inja_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    const auto source = find_source(services, pipelines, pipe.source).filter(make_event_filter(pipe.event));
    const auto transform = create_inja_pipeline(pipe.tmplate, { pipe.name });
    auto observer = source.map(transform).as_dynamic( );
    pipelines.push_back({ pipe.name, std::move(observer) });
  };
}

auto
make_jq_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::jq_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    const auto source = find_source(services, pipelines, pipe.source).filter(make_event_filter(pipe.event));
    const auto transform = create_jq_pipeline(pipe.script, { pipe.name });
    auto observer = source.flat_map(transform).as_dynamic( );
    pipelines.push_back({ pipe.name, std::move(observer) });
  };
}

auto
make_buffer_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::buffer_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    const auto source = find_source(services, pipelines, pipe.source).filter(make_event_filter(pipe.event));
    const auto trigger = find_source(services, pipelines, pipe.trigger_source).filter(make_event_filter(pipe.trigger_event));
    auto observer = create_buffer_pipeline(std::move(trigger), std::move(source), logger).as_dynamic( );
    pipelines.push_back({ pipe.name, std::move(observer) });
  };
}

auto
make_emit_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::emit_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    const auto source = find_source(services, pipelines, pipe.source).filter(make_event_filter(pipe.event));
    const auto transform = create_emit_pipeline(pipe.data, { pipe.name });
    auto observer = source.map(transform).as_dynamic( );
    pipelines.push_back({ pipe.name, std::move(observer) });
  };
}

auto
make_http_client_visitor(const services_t& services, pipelines_t& pipelines, const Logger& logger)
{
  return [&](const config::http_client_pipeline_t& pipe) {
    logger.info("Creating pipeline " + pipe.pipeline + " [" + pipe.name + "]");
    const auto source = find_source(services, pipelines, pipe.source).filter(make_event_filter(pipe.event));
    const auto transform = pipe.ssl
      ? create_http_client_ssl_pipeline({ pipe.name })
      : create_http_client_pipeline({ pipe.name });
    auto observer = source.map(transform).as_dynamic( );
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

  const auto inja_visitor = make_inja_visitor(services, pipelines, logger);
  const auto jq_visitor = make_jq_visitor(services, pipelines, logger);
  const auto buffer_visitor = make_buffer_visitor(services, pipelines, logger);
  const auto emit_visitor = make_emit_visitor(services, pipelines, logger);
  const auto http_client_visitor = make_http_client_visitor(services, pipelines, logger);

  const auto visitor = overloaded{ std::move(inja_visitor),
                                   std::move(jq_visitor),
                                   std::move(buffer_visitor),
                                   std::move(emit_visitor),
                                   std::move(http_client_visitor) };

  for (const configuration_t::pipeline_t& pipe : pipeline_config) {
    std::visit(visitor, pipe);
  }

  return pipelines;
}
}
