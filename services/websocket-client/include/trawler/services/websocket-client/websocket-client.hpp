#pragma once
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>

namespace trawler {
rxcpp::observable<class ServicePacket>
create_websocket_client(const std::shared_ptr<class ServiceContext>& context,
                        const std::string& host,
                        unsigned short port,
                        const Logger& logger = { "websocket-client" });
}
