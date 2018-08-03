#include <boost/asio/bind_executor.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/bind.hpp>
#include <rxcpp/rx.hpp>
#include <trawler/services/service-context.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/websocket-client/websocket-client.hpp>

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
using resolver_t = tcp::resolver;
using resolver_result_t = tcp::resolver::results_type;

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
 * make_address_resolver
 ******************************************************************************/
auto
make_address_resolver(const context_tp& context, const logger_t& logger, const host_t& host, const port_t& port)
{
  return [=]( ) {
    using result_t = std::tuple<resolver_result_t>;

    auto resolver = std::make_shared<resolver_t>(context->get_session_context( ));

    auto on_subscribe = [=](auto subscriber) {
      auto on_resolve = [=](error_t ec, const resolver_result_t& results) {
        if (ec) {
          logger.critical("Address resolution failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Address resolution successful");
        subscriber.on_next(std::make_tuple(results));
        subscriber.on_completed( );
      };

      resolver->async_resolve(host, port, std::move(on_resolve));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}
/*******************************************************************************
 * make_websocket_connector
 ******************************************************************************/
auto
make_websocket_connector(const context_tp& context, const logger_t& logger)
{
  return [=](const resolver_result_t& resolve_result) {
    using result_t = std::tuple<stream_tp>;

    auto stream = std::make_shared<stream_t>(context->get_session_context( ));

    auto on_subscribe = [=](auto subscriber) {
      auto on_connect = [=](error_t ec, auto /*endpoint*/) {
        if (ec) {
          logger.critical("Connection failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Connection successful");
        subscriber.on_next(std::make_tuple(stream));
        subscriber.on_completed( );
      };

      asio::async_connect(stream->next_layer( ), resolve_result, std::move(on_connect));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}

/*******************************************************************************
 * make_websocket_handshaker
 ******************************************************************************/
auto
make_websocket_handshaker(const logger_t& logger, const std::string& host, const std::string& target)
{
  return [=](const stream_tp& stream) {
    using result_t = std::tuple<stream_tp>;

    auto on_subscribe = [=](auto subscriber) {
      auto on_handshake = [=](error_t ec) {
        if (ec) {
          logger.critical("Handshake failed");
          subscriber.on_error(make_runtime_error(ec));
          return;
        }
        logger.debug("Handshake successful");
        subscriber.on_next(std::make_tuple(stream));
        subscriber.on_completed( );
      };

      stream->async_handshake(host, target, std::move(on_handshake));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}

/*******************************************************************************
 * run_websocket_event_loop
 ******************************************************************************/
template<typename DoRead>
void
run_websocket_event_loop(DoRead do_read)
{
  do_read([=](auto ec, auto logger, auto buffer, auto on_next, auto on_error, auto on_completed) {
    using status_t = ServicePacket::EStatus;
    logger.debug("Reading...");

    if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
      logger.info("Connection closed");
      on_next(status_t::DISCONNECTED);
      on_completed( );
      return;
    }

    if (ec) {
      logger.info("Error: " + ec.message( ));
      on_error(make_runtime_error(ec));
      return;
    }

    {
      auto data = beast::buffers_to_string(buffer->data( ));
      on_next(status_t::DATA_TRANSMISSION, std::move(data));
      buffer->consume(buffer->size( ));
    }

    run_websocket_event_loop(do_read);
  });
}

/*******************************************************************************
 * make_websocket_event_loop
 ******************************************************************************/
auto
make_websocket_event_loop(const context_tp& context, const logger_t& logger)
{
  return [=](const stream_tp& stream) {
    using result_t = ServicePacket;

    auto buffer = std::make_shared<beast::multi_buffer>( );
    auto session_strand = std::make_shared<strand_t>(context->get_session_context( ).get_executor( ));
    auto service_strand = std::make_shared<strand_t>(context->get_service_context( ).get_executor( ));

    auto on_subscribe = [=](auto subscriber) {
      logger.debug("Someone subscribed");

      using status_t = ServicePacket::EStatus;
      using data_t = const std::string&;

      auto on_write = [=](data_t data) {
        auto fn = [=] {
          auto cb = [](error_t, std::size_t) {};
          stream->async_write(asio::buffer(data), asio::bind_executor(*session_strand, std::move(cb)));
        };
        asio::bind_executor(*service_strand, fn)( );
      };

      auto on_error = [=](std::exception_ptr e) {
        auto fn = asio::bind_executor(*service_strand, [=] { subscriber.on_error(e); });
        fn( );
      };

      auto on_next = [=](status_t status, data_t data = "") {
        auto fn = asio::bind_executor(*service_strand, [=] {
          subscriber.on_next(ServicePacket{ status, data, on_write });
        });
        fn( );
      };

      auto on_completed = [=]( ) {
        auto fn = asio::bind_executor(*service_strand, [=]( ) { subscriber.on_completed( ); });
        fn( );
      };

      on_next(status_t::CONNECTED);

      auto do_read = [=](auto on_read_impl) {
        logger.debug("Performing read");

        auto read_callback = [=](error_t ec, std::size_t /*bytes_transferred*/) {
          on_read_impl(ec, logger, buffer, on_next, on_error, on_completed);
        };

        stream->async_read(*buffer, asio::bind_executor(*session_strand, read_callback));
      };

      run_websocket_event_loop(std::move(do_read));
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
                        const Logger& logger)
{
  using namespace rxcpp::operators;

  auto address_resolver = make_address_resolver(context, logger, host, std::to_string(port));
  auto websocket_connector = make_websocket_connector(context, logger);
  auto websocket_handshaker = make_websocket_handshaker(logger, host, "/");
  auto event_loop = make_websocket_event_loop(context, logger);

  return address_resolver( )
    .map([=](auto result) { return std::apply(websocket_connector, result); })
    .switch_on_next( )
    .map([=](auto result) { return std::apply(websocket_handshaker, result); })
    .switch_on_next( )
    .map([=](auto result) { return std::apply(event_loop, result); })
    .switch_on_next( )
    .lift<ServicePacket>([](auto subscriber) { return subscriber; })
    .as_dynamic( );
}
}
