#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <rxcpp/rx.hpp>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/make-runtime-error.hpp>
#include <trawler/services/service-context.hpp>

namespace trawler {
/*******************************************************************************
 * make_address_resolver
 ******************************************************************************/
inline auto
make_address_resolver(const std::shared_ptr<ServiceContext>& context,
                      const Logger& logger,
                      const std::string& host,
                      const std::string& port)
{
  return [=]( ) {
    using tcp = boost::asio::ip::tcp;
    using error_t = boost::system::error_code;
    using result_t = tcp::resolver::results_type;

    auto resolver = std::make_shared<tcp::resolver>(context->get_session_context( ));

    auto on_subscribe = [resolver, logger, host, port](auto subscriber) {
      auto on_resolve = [resolver, logger, subscriber](error_t ec, const result_t& results) {
        if (ec) {
          logger.critical("Address resolution failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Address resolution successful");
        subscriber.on_next(results);
        subscriber.on_completed( );
      };

      resolver->async_resolve(host, port, std::move(on_resolve));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}
}
