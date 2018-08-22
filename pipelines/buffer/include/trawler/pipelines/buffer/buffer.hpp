#pragma once
#include <functional>
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {
rxcpp::observable<ServicePacket>
create_buffer_pipeline(rxcpp::observable<ServicePacket> trigger,
                       rxcpp::observable<ServicePacket> source,
                       const Logger& logger = { "buffer-pipeline" });
}
