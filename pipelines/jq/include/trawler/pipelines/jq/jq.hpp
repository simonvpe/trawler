#pragma once
#include <functional>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {
std::function<ServicePacket(ServicePacket)>
create_jq_pipeline(const std::string& script, const Logger& logger = { "jq" });
}
