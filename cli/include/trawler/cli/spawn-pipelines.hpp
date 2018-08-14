#pragma once
#include <rxcpp/rx.hpp>
#include <trawler/cli/configuration.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

std::vector<rxcpp::subscription>
spawn_pipelines(const std::shared_ptr<class ServiceContext>& context,
                const std::vector<configuration_t::pipeline_t>& pipelines,
                const Logger& logger);
}