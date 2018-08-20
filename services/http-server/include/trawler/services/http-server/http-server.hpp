#pragma once
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>

namespace trawler {

rxcpp::observable<class ServicePacket>
create_http_server(const Logger& logger = { "http-server" });
}
