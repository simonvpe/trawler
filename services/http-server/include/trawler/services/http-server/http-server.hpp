#pragma once
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {
rxcpp::observable<ServicePacket>
create_http_server(const std::shared_ptr<ServiceContext>& context,
                   const std::string& host,
                   unsigned short port,
                   const Logger& logger = { "http-server" });
}
