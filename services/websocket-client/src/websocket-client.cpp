#include "make-address-resolver.hpp"
#include "make-websocket-handshaker.hpp"
#include <boost/asio/connect.hpp>
#include <rxcpp/rx.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>
#include <trawler/services/websocket-common/make-runtime-error.hpp>
#include <trawler/services/websocket-common/make-websocket-event-loop.hpp>

namespace trawler {

using context_t = ServiceContext;
using context_tp = std::shared_ptr<context_t>;
using error_t = boost::system::error_code;
using logger_t = Logger;
using stream_t = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;
using stream_tp = std::shared_ptr<stream_t>;

/*******************************************************************************
 * make_websocket_connector
 ******************************************************************************/
auto
make_websocket_connector(const context_tp& context, const logger_t& logger)
{
  return [=](const tcp::resolver::results_type& resolve_result) {
    using result_t = stream_tp;

    auto stream = std::make_shared<stream_t>(context->get_session_context( ));

    auto on_subscribe = [=](auto subscriber) {
      auto on_connect = [logger, stream, subscriber, context](error_t ec, auto /*endpoint*/) {
        if (ec) {
          logger.critical("Connection failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Connection successful");
        subscriber.on_next(stream);
        subscriber.on_completed( );
      };

      boost::asio::async_connect(stream->next_layer( ), resolve_result, std::move(on_connect));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}

/*******************************************************************************
 * create_websocket_client
 ******************************************************************************/
rxcpp::observable<ServicePacket>
create_websocket_client(const std::shared_ptr<ServiceContext>& context,
                        const std::string& host,
                        unsigned short port,
                        const std::string& target,
                        const Logger& logger)
{
  using namespace rxcpp::operators;

  auto address_resolver = make_address_resolver(context, logger, host, std::to_string(port));
  auto websocket_connector = make_websocket_connector(context, logger);
  auto websocket_handshaker = make_websocket_handshaker<stream_t>(logger, host, target);
  auto event_loop = make_websocket_event_loop<stream_t>(context, logger);

  return address_resolver( )
    .flat_map(std::move(websocket_connector))
    .flat_map(std::move(websocket_handshaker))
    .flat_map(std::move(event_loop));
}
}
