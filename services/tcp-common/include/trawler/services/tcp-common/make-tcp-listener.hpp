#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <memory>
#include <rxcpp/rx.hpp>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/make-runtime-error.hpp>
#include <trawler/services/service-context.hpp>

namespace trawler {
/*******************************************************************************
 * make_tcp_listener
 ******************************************************************************/
inline
auto
make_tcp_listener(const std::shared_ptr<ServiceContext>& context,
                  const Logger& logger,
                  const std::string& host,
                  const unsigned short port)
{
  using acceptor_t = boost::asio::ip::tcp::acceptor;
  using acceptor_tp = std::shared_ptr<acceptor_t>;
  using endpoint_t = boost::asio::ip::tcp::endpoint;

  return [=] {
    using result_t = acceptor_tp;

    auto on_subscribe = [=](auto subscriber) {
      auto acceptor = std::make_shared<acceptor_t>(context->get_session_context( ));

      boost::system::error_code ec;
      auto endpoint = endpoint_t{ boost::asio::ip::make_address(host), port };

      acceptor->open(endpoint.protocol( ), ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      acceptor->set_option(boost::asio::socket_base::reuse_address(true), ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      acceptor->bind(endpoint, ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      logger.info("Listening for connections");
      subscriber.on_next(acceptor);
      subscriber.on_completed( );
    };
    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}
}
