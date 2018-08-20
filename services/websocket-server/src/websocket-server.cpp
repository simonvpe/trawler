#include <trawler/services/make-runtime-error.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/tcp-common/make-tcp-listener.hpp>
#include <trawler/services/websocket-common/make-websocket-event-loop.hpp>
#include <trawler/services/websocket-server/websocket-server.hpp>

namespace trawler {

namespace {

using acceptor_t = boost::asio::ip::tcp::acceptor;
using acceptor_tp = std::shared_ptr<acceptor_t>;
using context_t = ServiceContext;
using context_tp = std::shared_ptr<ServiceContext>;
using error_t = boost::system::error_code;
using host_t = std::string;
using logger_t = Logger;
using port_t = unsigned short;
using socket_t = boost::asio::ip::tcp::socket;
using socket_tp = std::shared_ptr<socket_t>;
using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;
using stream_t = boost::beast::websocket::stream<tcp::socket>;
using stream_tp = std::shared_ptr<stream_t>;

/*******************************************************************************
 * make_tcp_acceptor
 ******************************************************************************/
template<typename DoAccept>
void
run_acceptor_loop(DoAccept do_accept)
{
  do_accept([=](auto ec, auto logger, auto socket, auto on_next, auto on_error, auto on_completed) {
    if (ec == boost::system::errc::operation_canceled) {
      on_completed( );
      return;
    }

    if (ec) {
      on_error(make_runtime_error(ec));
      return;
    }

    const auto address = socket->remote_endpoint( ).address( );
    logger.info("Client connected!");

    on_next(std::move(socket));

    run_acceptor_loop(do_accept);
  });
}

/*******************************************************************************
 * make_tcp_acceptor
 ******************************************************************************/
auto
make_tcp_acceptor(const std::shared_ptr<ServiceContext>& context, const Logger& logger)
{
  return [=](acceptor_tp acceptor) {
    using result_t = socket_tp;

    auto on_subscribe = [=](auto subscriber) {
      auto service_strand = std::make_shared<strand_t>(context->get_service_context( ).get_executor( ));

      auto on_error = [service_strand, subscriber](std::exception_ptr e) {
        auto fn = boost::asio::bind_executor(*service_strand, [=] { subscriber.on_error(e); });
        fn( );
      };

      auto on_next = [service_strand, subscriber](socket_tp socket) {
        auto fn = boost::asio::bind_executor(*service_strand, [=] { subscriber.on_next(socket); });
        fn( );
      };

      auto on_completed = [service_strand, subscriber] {
        auto fn = boost::asio::bind_executor(*service_strand, [=] { subscriber.on_completed( ); });
        fn( );
      };

      auto do_accept = [=](auto on_accept_impl) {
        auto socket = std::make_shared<socket_t>(context->get_session_context( ));
        auto on_accept = [=](error_t ec) { on_accept_impl(ec, logger, socket, on_next, on_error, on_completed); };
        acceptor->async_accept(*socket, std::move(on_accept));
      };
      run_acceptor_loop(std::move(do_accept));
    };
    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}

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
