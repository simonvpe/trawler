#pragma once
#include <functional>
#include <optional>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

std::function<void(const ServicePacket&)>
create_endpoint(const std::optional<std::string>& data, const Logger& logger = { "endpoint" });
}
