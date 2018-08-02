#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/bind.hpp>
#include <memory>
#include <rxcpp/rx.hpp>
#include <string>
#include <trawler/services/packet-handler.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/util/create-asio-service.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>
#include <utility>

namespace trawler {
namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

namespace {

class WebsocketClientSession : public std::enable_shared_from_this<WebsocketClientSession>
{
  tcp::resolver resolver;
  websocket::stream<tcp::socket> ws;
  beast::multi_buffer buffer;
  asio::strand<asio::io_context::executor_type> read_strand;
  asio::strand<asio::io_context::executor_type> write_strand;
  std::string host;
  std::string port;
  PacketHandler packet_handler;
  Logger logger;

  void _fail_on_error(boost::system::error_code ec)
  {
    try {
      if (ec) {
        logger.critical(ec.message( ));
        throw std::runtime_error(ec.message( ));
      }
    } catch (...) {
      std::rethrow_exception(std::current_exception( ));
    }
  }

  void _on_resolve(boost::system::error_code ec, const tcp::resolver::results_type& results)
  {
    _fail_on_error(ec);
    auto me = shared_from_this( );
    auto fn = boost::bind(&WebsocketClientSession::_on_connect, std::move(me), _1);
    asio::async_connect(ws.next_layer( ), results.begin( ), results.end( ), std::move(fn));
  }

  void _on_connect(boost::system::error_code ec)
  {
    _fail_on_error(ec);
    auto me = shared_from_this( );
    auto fn = boost::bind(&WebsocketClientSession::_on_handshake, std::move(me), _1);
    ws.async_handshake(host, "/", std::move(fn));
  }

  void _on_handshake(boost::system::error_code ec)
  {
    _fail_on_error(ec);
    logger.info("WebsocketClientSession: Connected to server");
    auto me = shared_from_this( );
    auto reply_fn = [me](std::string write_data) { me->write(std::move(write_data)); };
    packet_handler.call_on_data(ServicePacket{ ServicePacket::EStatus::CONNECTED, "", std::move(reply_fn) });
    _do_read( );
  }

  void _do_read( )
  {
    auto fn = [me = shared_from_this( )](boost::system::error_code ec, std::size_t bytes_transferred) {
      boost::ignore_unused(bytes_transferred);

      if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
        const auto status = ServicePacket::EStatus::DISCONNECTED;
        me->disconnect( );
        me->logger.info("WebsocketClientSession: Connection closed [use count " + std::to_string(me.use_count( )) +
                        "]");
        // Use count = 2
        me->packet_handler.call_on_data(ServicePacket{ status });
        me->packet_handler.call_on_closed( );
        return;
      }

      try {
        const auto status = ServicePacket::EStatus::DATA_TRANSMISSION;
        me->_fail_on_error(ec);
        me->logger.debug("WebsocketClientSession: Received data from server [use count " +
                         std::to_string(me.use_count( )) + "]");
        auto str = beast::buffers_to_string(me->buffer.data( ));
        auto reply_fn = [me](std::string write_data) { me->write(std::move(write_data)); };
        auto packet = ServicePacket{ status, std::move(str), std::move(reply_fn) };
        me->packet_handler.call_on_data(std::move(packet));
        me->buffer.consume(me->buffer.size( ));
        me->_do_read( );
      } catch (...) {
        const auto status = ServicePacket::EStatus::DISCONNECTED;
        auto reply_fn = [](std::string) { throw std::runtime_error{ "Trying to write to disconnected client" }; };
        me->disconnect( );
        me->packet_handler.call_on_data(ServicePacket{ status, "", std::move(reply_fn) });
        me->packet_handler.call_on_error(std::current_exception( ));
        return;
      }
    };
    ws.async_read(buffer, asio::bind_executor(read_strand, std::move(fn)));
  }

public:
  WebsocketClientSession(asio::io_context& context, std::string host, unsigned short port, Logger logger)
    : resolver{ context }
    , ws{ context }
    , read_strand{ context.get_executor( ) }
    , write_strand{ context.get_executor( ) }
    , host{ std::move(host) }
    , port{ std::to_string(port) }
    , logger{ std::move(logger) }
  {
    this->logger.debug("WebsocketClientSession()");
  }

  ~WebsocketClientSession( ) { logger.debug("~WebsocketClientSession()"); }

  WebsocketClientSession(const WebsocketClientSession&) = delete;
  WebsocketClientSession(WebsocketClientSession&&) = delete;
  WebsocketClientSession& operator=(const WebsocketClientSession&) = delete;
  WebsocketClientSession& operator=(WebsocketClientSession&&) = delete;

  void run(PacketHandler packet_handler)
  {
    auto me = shared_from_this( );
    logger.debug("WebsocketClientSession::run() [use count " + std::to_string(me.use_count( )) + "]");
    // Use count = 2
    this->packet_handler = std::move(packet_handler);
    auto fn = boost::bind(&WebsocketClientSession::_on_resolve, std::move(me), _1, _2);
    resolver.async_resolve(host.c_str( ), port.c_str( ), std::move(fn));
  }

  void disconnect( )
  {
    asio::bind_executor(write_strand, [me = shared_from_this( )] {
      if (!me->ws.is_open( )) {
        return;
      }

      me->logger.debug("WebsocketClientSession::disconnect() [use count " + std::to_string(me.use_count( )) + "]");

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
    if (ws.is_open( )) {
      auto me = shared_from_this( );
      logger.debug("WebsocketClientSession::write() [use count " + std::to_string(me.use_count( )) + "]");
      auto fn = [me](boost::system::error_code ec, std::size_t bytes_transferred) {
        if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
          me->disconnect( );
          return;
        }
        me->_fail_on_error(ec);
      };
      ws.async_write(asio::buffer(data), asio::bind_executor(write_strand, std::move(fn)));
    }
  }
};
}

rxcpp::observable<ServicePacket>
create_websocket_client(const std::shared_ptr<ServiceContext>& context,
                        const std::string& host,
                        unsigned short port,
                        const Logger& logger)
{

  using session_t = WebsocketClientSession;
  using lifetime_t = lifetime_type<session_t>;
  using endpoint_t = lifetime_t::endpoint_type;
  using strand_t = lifetime_t::strand_type;

  auto session = std::make_shared<session_t>(context->get_session_context( ), host, port, logger);

  auto lifetime = std::make_shared<lifetime_t>(
    std::move(session), strand_t{ context->get_service_context( ).get_executor( ) }, EStatus::IDLE, context);

  return create_asio_service<session_t>(std::move(lifetime));
}
}
