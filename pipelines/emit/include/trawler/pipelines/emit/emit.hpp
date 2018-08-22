#pragma once
#include <functional>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {
std::function<ServicePacket(ServicePacket)>
create_emit_pipeline(const std::string& data, const Logger& logger = { "emit-pipeline" });
}
