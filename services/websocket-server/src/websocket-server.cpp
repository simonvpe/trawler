#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/bind.hpp>
#include <functional>
#include <memory>
#include <trawler/services/service-context.hpp>
#include <trawler/services/util/create-asio-service.hpp>
#include <trawler/services/util/tcp-listener.hpp>
#include <trawler/services/websocket-server/websocket-server.hpp>
#include <tuple>
#include <utility>

namespace trawler {

namespace beast = boost::beast;
namespace websocket = beast::websocket;

namespace {

class WebsocketServerSession : public std::enable_shared_from_this<WebsocketServerSession>
{
  websocket::stream<tcp::socket> ws;
  asio::strand<asio::io_context::executor_type> read_strand;
  asio::strand<asio::io_context::executor_type> write_strand;
  beast::multi_buffer buffer;
  PacketHandler packet_handler;
  std::function<void(std::shared_ptr<WebsocketServerSession>)> erase_from_pool;
  Logger logger;

  void _fail_on_error(boost::system::error_code ec)
  {
    try {
      if (ec) {
        logger.critical(ec.message( ));
        throw std::runtime_error(ec.message( ));
      }
    } catch (...) {
      auto me = shared_from_this( );
      me->erase_from_pool(me);
      std::rethrow_exception(std::current_exception( ));
    }
  }

  void _do_read( )
  {
    auto me = this->shared_from_this( );
    auto fn = [me](boost::system::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);

      if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
        const auto status = ServicePacket::EStatus::DISCONNECTED;
        me->disconnect( );
        me->logger.info("WebsocketServerSession: Connection was closed");
        me->erase_from_pool(me);
        me->packet_handler.call_on_data(ServicePacket{ status });
        me->packet_handler.call_on_closed( );
        return;
      }

      try {
        const auto status = ServicePacket::EStatus::DATA_TRANSMISSION;
        me->_fail_on_error(ec);
        me->logger.debug("WebsocketServerSession: Received data from client");
        auto str = beast::buffers_to_string(me->buffer.data( ));
        auto reply_fn = [me](std::string write_data) { me->write(std::move(write_data)); };
        auto packet = ServicePacket{ status, std::move(str), std::move(reply_fn) };
        me->buffer.consume(me->buffer.size( ));
        me->packet_handler.call_on_data(std::move(packet));
      } catch (...) {
        const auto status = ServicePacket::EStatus::DISCONNECTED;
        me->disconnect( );
        me->packet_handler.call_on_data(ServicePacket{ status });
        me->packet_handler.call_on_error(std::current_exception( ));
        return;
      }

      me->_do_read( );
    };
    ws.async_read(buffer, asio::bind_executor(read_strand, std::move(fn)));
  }

public:
  explicit WebsocketServerSession(tcp::socket socket, PacketHandler packet_handler, Logger logger)
    : ws{ std::move(socket) }
    , read_strand{ ws.get_executor( ) }
    , write_strand{ ws.get_executor( ) }
    , packet_handler{ std::move(packet_handler) }
    , logger{ std::move(logger) }
  {
    this->logger.debug("WebsocketServerSession()");
  }

  ~WebsocketServerSession( ) { logger.debug("~WebsocketServerSession()"); }

  WebsocketServerSession(const WebsocketServerSession&) = delete;
  WebsocketServerSession(WebsocketServerSession&&) = delete;
  WebsocketServerSession& operator=(const WebsocketServerSession&) = delete;
  WebsocketServerSession& operator=(WebsocketServerSession&&) = delete;

  void run(std::function<void(std::shared_ptr<WebsocketServerSession>)> erase_from_pool)
  {
    logger.info("WebsocketServerSession::run()");
    this->erase_from_pool = std::move(erase_from_pool);

    packet_handler.call_on_data(ServicePacket{ ServicePacket::EStatus::CONNECTED });

    auto me = this->shared_from_this( );
    auto fn = [me](boost::system::error_code ec) {
      me->_fail_on_error(ec);
      me->_do_read( );
    };
    ws.async_accept(asio::bind_executor(read_strand, std::move(fn)));
  }

  void disconnect( )
  {
    auto me = shared_from_this( );
    asio::bind_executor(write_strand, [me] {
      if (!me->ws.is_open( )) {
        return;
      }

      me->logger.debug("WebsocketServerSession::disconnect()");

      try {
        boost::system::error_code ec;
        me->ws.close(websocket::close_code::normal);

        beast::multi_buffer drain; // TODO: drain_buffer

        while (true) {
          me->ws.read(drain, ec);

          if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled ||
              ec == asio::error::eof) {
            break;
          }
          me->_fail_on_error(ec);
        }
      } catch (...) {
        /* If closing fails chances are that the socket was already closed */
      }
    })( );
  }

  void write(std::string data)
  {
    logger.debug("WebsocketServerSession::write()");
    auto me = this->shared_from_this( );
    auto fn = [me](boost::system::error_code ec, std::size_t bytes_transferred) {
      if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
        me->disconnect( );
        return;
      }
      me->_fail_on_error(ec);
    };
    ws.async_write(asio::buffer(data), asio::bind_executor(write_strand, fn));
  }
};
}

rxcpp::observable<ServicePacket>
create_websocket_server2(const std::shared_ptr<ServiceContext>& context,
                         const std::string& host,
                         unsigned short port,
                         const Logger& logger)
{
  using session_t = TcpListener<WebsocketServerSession>;
  using lifetime_t = lifetime_type<session_t>;
  using endpoint_t = lifetime_t::endpoint_type;
  using strand_t = lifetime_t::strand_type;

  auto session = std::make_shared<session_t>(
    context->get_session_context( ), endpoint_t{ boost::asio::ip::make_address(host), port }, logger);

  auto lifetime = std::make_shared<lifetime_t>(
    std::move(session), strand_t{ context->get_service_context( ).get_executor( ) }, EStatus::IDLE, context);

  return create_asio_service<session_t>(std::move(lifetime));
}

using error_t = boost::system::error_code;
using acceptor_t = tcp::acceptor;
using acceptor_tp = std::shared_ptr<acceptor_t>;
using socket_t = tcp::socket;
using socket_tp = std::shared_ptr<socket_t>;
using strand_t = asio::strand<asio::io_context::executor_type>;
using stream_t = websocket::stream<tcp::socket>;
using stream_tp = std::shared_ptr<stream_t>;

namespace {
/*******************************************************************************
 * make_runtime_error
 ******************************************************************************/
auto
make_runtime_error(const std::string& message)
{
  return std::make_exception_ptr(std::runtime_error{ message });
}

auto
make_runtime_error(const error_t& e)
{
  return make_runtime_error(e.message( ));
}

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
    using result_t = std::tuple<acceptor_tp>;

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
      subscriber.on_next(std::make_tuple(acceptor));
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

    on_next(socket);

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
    using result_t = std::tuple<socket_tp>;

    auto on_subscribe = [=](auto subscriber) {
      auto service_strand = std::make_shared<strand_t>(context->get_service_context( ).get_executor( ));

      auto on_error = [service_strand, subscriber](std::exception_ptr e) {
        auto fn = asio::bind_executor(*service_strand, [=] { subscriber.on_error(e); });
        fn( );
      };

      auto on_next = [service_strand, subscriber](socket_tp socket) {
        auto fn = asio::bind_executor(*service_strand, [=] { subscriber.on_next(socket); });
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
    using result_t = std::tuple<socket_tp, stream_tp>;

    auto stream = std::make_shared<stream_t>(*socket);

    auto on_subscribe = [stream, socket, context, logger](auto subscriber) {
      logger.debug("Accepting websocket");
      auto fn = [=](error_t ec) {
        if (ec) {
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Accepted websocket");
        subscriber.on_next(std::make_tuple(socket, stream));
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

  tcp_listener( )
    .flat_map([=](auto result) { return tcp_acceptor(std::get<0>(result)); })
    .flat_map([=](auto result) { return websocket_acceptor(std::get<0>(result)); })
    .subscribe([](auto) { std::cout << "DONE\n"; });

  return rxcpp::observable<>::create<ServicePacket>([](auto subscriber) {});
}
}
