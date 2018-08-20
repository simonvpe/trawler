#pragma once
#include <rxcpp/rx.hpp>
#include <trawler/cli/configuration.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>
spawn_pipelines(const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& services,
                const std::vector<configuration_t::pipeline_t>& pipeline_config,
                const Logger& logger);
}
