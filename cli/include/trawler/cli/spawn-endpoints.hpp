#pragma once
#include <rxcpp/rx.hpp>
#include <trawler/cli/configuration.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

std::vector<rxcpp::subscription>
spawn_endpoints(const std::shared_ptr<class ServiceContext>& context,
                const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& services,
                const std::vector<std::pair<std::string, rxcpp::observable<ServicePacket>>>& pipelines,
                const std::vector<configuration_t::endpoint_t>& endpoint_config,
                const Logger& logger);
}
