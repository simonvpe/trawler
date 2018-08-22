#pragma once
#include <functional>
#include <optional>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

std::function<void(const ServicePacket&)>
create_endpoint(const Logger& logger = { "endpoint" });
}
