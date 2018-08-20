#include <trawler/services/make-runtime-error.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/tcp-common/make-tcp-acceptor.hpp>
#include <trawler/services/tcp-common/make-tcp-listener.hpp>
#include <trawler/services/websocket-common/make-websocket-event-loop.hpp>
#include <trawler/services/websocket-server/websocket-server.hpp>

namespace trawler {

namespace {

using error_t = boost::system::error_code;
using socket_t = boost::asio::ip::tcp::socket;
using socket_tp = std::shared_ptr<socket_t>;
using stream_t = boost::beast::websocket::stream<tcp::socket>;
using stream_tp = std::shared_ptr<stream_t>;

/*******************************************************************************
 * make_websocket_acceptor
 ******************************************************************************/
auto
make_websocket_acceptor(const std::shared_ptr<ServiceContext>& context, const Logger& logger)
{
  return [=](socket_tp socket) {
    using result_t = stream_tp;

    auto stream = std::make_shared<stream_t>(std::move(*socket));

    auto on_subscribe = [stream, context, logger](auto subscriber) {
      auto fn = [stream, context, logger, subscriber](error_t ec) {
        if (ec) {
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        subscriber.on_next(stream);
        subscriber.on_completed( );
      };
      stream->async_accept(boost::asio::bind_executor(context->get_service_context( ), fn));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}
}

rxcpp::observable<ServicePacket>
create_websocket_server(const std::shared_ptr<ServiceContext>& context,
                        const std::string& host,
                        unsigned short port,
                        const Logger& logger)
{
  auto tcp_listener = make_tcp_listener(context, logger, host, port);
  auto tcp_acceptor = make_tcp_acceptor(context, logger);
  auto websocket_acceptor = make_websocket_acceptor(context, logger);
  auto websocket_event_loop = make_websocket_event_loop<stream_t>(context, logger);

  return tcp_listener( )
    .flat_map(std::move(tcp_acceptor))
    .flat_map(std::move(websocket_acceptor))
    .flat_map(std::move(websocket_event_loop));
}
}
