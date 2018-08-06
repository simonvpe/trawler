#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <memory>
#include <trawler/services/service-context.hpp>
#include <trawler/services/websocket-server/websocket-server.hpp>
#include <trawler/services/websocket-common/make-runtime-error.hpp>
#include <trawler/services/websocket-common/make-websocket-event-loop.hpp>
#include <tuple>
#include <utility>

namespace trawler {

namespace beast = boost::beast;
namespace websocket = beast::websocket;

namespace {

using error_t = boost::system::error_code;
using acceptor_t = tcp::acceptor;
using acceptor_tp = std::shared_ptr<acceptor_t>;
using socket_t = tcp::socket;
using socket_tp = std::shared_ptr<socket_t>;
using strand_t = asio::strand<asio::io_context::executor_type>;
using stream_t = websocket::stream<tcp::socket>;
using stream_tp = std::shared_ptr<stream_t>;
using logger_t = Logger;
using context_t = ServiceContext;
using context_tp = std::shared_ptr<ServiceContext>;

/*******************************************************************************
 * make_tcp_listener
 ******************************************************************************/
auto
make_tcp_listener(const std::shared_ptr<ServiceContext>& context,
                  const Logger& logger,
                  const std::string& host,
                  const unsigned short port)
{
  return [=] {
    using result_t = acceptor_tp;

    auto on_subscribe = [=](auto subscriber) {
      auto acceptor = std::make_shared<acceptor_t>(context->get_session_context( ));

      boost::system::error_code ec;
      auto endpoint = tcp::endpoint{ boost::asio::ip::make_address(host), port };

      acceptor->open(endpoint.protocol( ), ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      acceptor->set_option(asio::socket_base::reuse_address(true), ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      acceptor->bind(endpoint, ec);
      if (ec) {
        subscriber.on_error(make_runtime_error(ec));
        return;
      }

      acceptor->listen(asio::socket_base::max_listen_connections, ec);
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
        auto fn = asio::bind_executor(*service_strand, [=] { subscriber.on_error(e); });
        fn( );
      };

      auto on_next = [service_strand, subscriber](socket_tp socket) {
        auto fn = asio::bind_executor(*service_strand, [=] { subscriber.on_next(std::move(socket)); });
        fn( );
      };

      auto on_completed = [service_strand, subscriber] {
        auto fn = asio::bind_executor(*service_strand, [=] { subscriber.on_completed( ); });
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
 * make_websocket_handshaker
 ******************************************************************************/
auto
make_websocket_acceptor(const std::shared_ptr<ServiceContext>& context, const Logger& logger)
{
  return [=](socket_tp socket) {
    using result_t = stream_tp;

    auto stream = std::make_shared<stream_t>(std::move(*socket));

    auto on_subscribe = [stream, context, logger](auto subscriber) {
      logger.debug("Accepting websocket");
      auto fn = [=](error_t ec) {
        if (ec) {
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Accepted websocket");
        subscriber.on_next(stream);
        subscriber.on_completed( );
      };
      stream->async_accept(asio::bind_executor(context->get_service_context( ), fn));
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
  auto websocket_event_loop = make_websocket_event_loop(context, logger);
  
  return tcp_listener( )
    .flat_map(tcp_acceptor)
    .flat_map(websocket_acceptor)
    .flat_map(websocket_event_loop);
}
}
