#include "make-address-resolver.hpp"
#include "make-websocket-handshaker.hpp"
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/bind.hpp>
#include <rxcpp/rx.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>
#include <trawler/services/websocket-common/make-runtime-error.hpp>
#include <trawler/services/websocket-common/make-websocket-event-loop.hpp>

namespace trawler {
/*******************************************************************************
 * aliases
 ******************************************************************************/
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

/*******************************************************************************
 * type declarations
 ******************************************************************************/
using error_t = boost::system::error_code;
using host_t = std::string;
using port_t = std::string;
using context_t = ServiceContext;
using context_tp = std::shared_ptr<context_t>;
using logger_t = Logger;
using strand_t = asio::strand<asio::io_context::executor_type>;
using stream_t = websocket::stream<tcp::socket>;
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
      auto on_connect = [=](error_t ec, auto /*endpoint*/) {
        if (ec) {
          logger.critical("Connection failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Connection successful");
        subscriber.on_next(stream);
        subscriber.on_completed( );
      };

      asio::async_connect(stream->next_layer( ), resolve_result, std::move(on_connect));
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
    .flat_map(websocket_connector)
    .flat_map(websocket_handshaker)
    .flat_map(event_loop)
    .as_dynamic( );
}
}
