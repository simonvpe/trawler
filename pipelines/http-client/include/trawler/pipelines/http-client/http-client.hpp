#pragma once
#include <functional>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {
std::function<ServicePacket(ServicePacket)>
create_http_client_pipeline(const Logger& logger = { "http-client" });

std::function<ServicePacket(ServicePacket)>
create_http_client_ssl_pipeline(const Logger& logger = { "http-client" });
}
