#pragma once
#include <memory>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/make-runtime-error.hpp>

namespace trawler {
/*******************************************************************************
 * make_websocket_handshaker
 ******************************************************************************/
template<typename Stream>
inline auto
make_websocket_handshaker(const Logger& logger, const std::string& host, const std::string& target)
{
  using stream_tp = std::shared_ptr<Stream>;
  using error_t = boost::system::error_code;

  return [=](const stream_tp& stream) {
    using result_t = stream_tp;

    auto on_subscribe = [=](auto subscriber) {
      auto on_handshake = [=](error_t ec) {
        if (ec) {
          logger.critical("Handshake failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Handshake successful");
        subscriber.on_next(stream);
        subscriber.on_completed( );
      };

      stream->async_handshake(host, target, std::move(on_handshake));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}
}
