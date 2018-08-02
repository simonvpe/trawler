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
create_websocket_server(const std::shared_ptr<ServiceContext>& context,
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
}
