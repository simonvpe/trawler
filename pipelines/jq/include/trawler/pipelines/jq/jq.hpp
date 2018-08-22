#pragma once
#include <functional>
#include <rxcpp/rx.hpp>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

std::function<rxcpp::observable<ServicePacket>(const ServicePacket&)>
create_jq_pipeline(const std::string& script, const Logger& logger = { "jq" });
}
