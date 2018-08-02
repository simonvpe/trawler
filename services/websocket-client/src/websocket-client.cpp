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

/*******************************************************************************
 * state_type
 ******************************************************************************/

struct state_type
{
  using strand_type = asio::strand<asio::io_context::executor_type>;
  using stream_type = websocket::stream<tcp::socket>;
  using resolver_type = tcp::resolver;
  using buffer_type = beast::multi_buffer;
  using logger_type = Logger;
  using context_type = std::shared_ptr<ServiceContext>;

  resolver_type resolver;
  stream_type ws;
  buffer_type buffer;
  strand_type strand;
  strand_type write_strand;
  logger_type logger;
  context_type context;

  explicit state_type(decltype(context) context, decltype(logger) logger)
    : resolver{ context->get_session_context( ) }
    , ws{ context->get_session_context( ) }
    , strand{ context->get_session_context( ).get_executor( ) }
    , write_strand{ context->get_session_context( ).get_executor( ) }
    , logger{ std::move(logger) }
    , context{ std::move(context) }
  {}
};

using error_t = boost::system::error_code;

/*******************************************************************************
 * resolve_address
 ******************************************************************************/
using resolve_result_t = std::tuple<std::shared_ptr<state_type>, tcp::resolver::results_type>;

auto
resolve_address(std::shared_ptr<state_type> state, const std::string& host, const std::string& port)
{
  return rxcpp::observable<>::create<resolve_result_t>([state = std::move(state), host, port](auto subscriber) {
    auto on_resolve = [state, subscriber](error_t ec, const tcp::resolver::results_type& results) {
      if (ec) {
        try {
          throw std::runtime_error(ec.message( ));
        } catch (...) {
          subscriber.on_error(std::current_exception( ));
        }
      }
      state->logger.debug("Address resolution successful");
      subscriber.on_next(resolve_result_t{ std::move(state), results });
      subscriber.on_completed( );
    };
    state->resolver.async_resolve(host, port, std::move(on_resolve));
  });
}

/*******************************************************************************
 * connect
 ******************************************************************************/
using connect_result_t = std::tuple<std::shared_ptr<state_type>, tcp::endpoint>;

auto
connect(std::shared_ptr<state_type> state, const tcp::resolver::results_type& results)
{
  return rxcpp::observable<>::create<connect_result_t>([state = std::move(state), results](auto subscriber) {
    auto on_connect = [state, subscriber](error_t ec, const tcp::endpoint& endpoint) {
      if (ec) {
        try {
          throw std::runtime_error(ec.message( ));
        } catch (...) {
          subscriber.on_error(std::current_exception( ));
        }
      }
      state->logger.debug("Connection successful");
      subscriber.on_next(connect_result_t{ std::move(state), endpoint });
      subscriber.on_completed( );
    };
    asio::async_connect(state->ws.next_layer( ), results, std::move(on_connect));
  });
}

/*******************************************************************************
 * websocket_handshake
 ******************************************************************************/
using websocket_handshake_result_t = std::shared_ptr<state_type>;

auto
websocket_handshake(std::shared_ptr<state_type> state, const std::string host, const std::string target)
{
  using result_t = websocket_handshake_result_t;
  auto on_subscribe = [state = std::move(state), host, target](rxcpp::subscriber<result_t> subscriber) {
    auto on_handshake = [state, subscriber](error_t ec) {
      if (ec) {
        try {
          throw std::runtime_error(ec.message( ));
        } catch (...) {
          subscriber.on_error(std::current_exception( ));
        }
      }
      state->logger.debug("Websocket handshake successful");
      subscriber.on_next(websocket_handshake_result_t{ std::move(state) });
      subscriber.on_completed( );
    };
    state->ws.async_handshake(host, target, std::move(on_handshake));
  };
  return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
}

/*******************************************************************************
 * make_writer
 ******************************************************************************/
auto
make_websocket_writer(std::shared_ptr<state_type> state)
{
  return asio::bind_executor(state->write_strand, [state](std::string data) {
    auto on_write = [state](error_t ec, std::size_t bytes_transferred) {
      if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
        return;
      }
    };
    state->ws.async_write(asio::buffer(data), asio::bind_executor(state->write_strand, std::move(on_write)));
  });
};

/*******************************************************************************
 * start_websocket_loop
 ******************************************************************************/
void
start_websocket_loop(std::shared_ptr<state_type> state, rxcpp::subscriber<ServicePacket> subscriber)
{
  asio::bind_executor(state->strand, [state, subscriber = std::move(subscriber)] {
    auto on_read = [state, subscriber](error_t ec, std::size_t bytes_transferred) {
      if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled || ec == asio::error::eof) {
        state->logger.info("Connection closed");
        subscriber.on_next(ServicePacket{ ServicePacket::EStatus::DISCONNECTED });
        subscriber.on_completed( );
        return;
      }

      if (ec) {
        state->logger.info("Error: " + ec.message( ));
        subscriber.on_error(std::make_exception_ptr(std::runtime_error{ ec.message( ) }));
        return;
      }

      state->logger.debug("Reading...");

      auto data = beast::buffers_to_string(state->buffer.data( ));
      auto packet = ServicePacket{ ServicePacket::EStatus::DATA_TRANSMISSION,
                                   std::move(data),
                                   make_websocket_writer(state) };
      subscriber.on_next(std::move(packet));
      state->buffer.consume(state->buffer.size( ));

      start_websocket_loop(std::move(state), std::move(subscriber));
    };
    state->ws.async_read(state->buffer, asio::bind_executor(state->strand, std::move(on_read)));
  })( );
}

/*******************************************************************************
 * disconnect_websocket
 ******************************************************************************/
void
disconnect_websocket(std::shared_ptr<state_type> state, rxcpp::subscriber<ServicePacket> subscriber)
{
  auto do_disconnect = [state, subscriber = std::move(subscriber)] {
    state->ws.close(websocket::close_code::normal);
    asio::bind_executor(state->strand, [state, subscriber] {
      beast::multi_buffer drain;
      while (true) {

        boost::system::error_code ec;

        state->ws.read(drain, ec);

        if (ec == websocket::error::closed || ec == boost::system::errc::operation_canceled) {
          state->logger.info("Disconnected");
          return;
        }

        if (ec) {
          state->logger.debug("Failed to disconnect: " + ec.message( ));
          subscriber.on_error(std::make_exception_ptr(std::runtime_error{ ec.message( ) }));
        }
      }
    })( );
  };
  asio::bind_executor(state->write_strand, std::move(do_disconnect))( );
}

/*******************************************************************************
 * create_websocket_observable
 ******************************************************************************/
using read_result_t = ServicePacket;

auto
create_websocket_observable(std::shared_ptr<state_type> state)
{
  using result_t = read_result_t;
  auto on_subscribe = [state = std::move(state)](rxcpp::subscriber<result_t> subscriber) {
    subscriber.on_next(ServicePacket{ ServicePacket::EStatus::CONNECTED, "", make_websocket_writer(state) });
    using read_t = std::function<void(std::shared_ptr<state_type>)>;

    start_websocket_loop(std::move(state), std::move(subscriber));
  };
  return rxcpp::observable<>::create<ServicePacket>(on_subscribe);
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

  auto state = std::make_shared<state_type>(context, logger);

  // Resolve the address
  return resolve_address(state, host, std::to_string(port))
         // Connect to the resolved address
         | map([](const resolve_result_t& resolve_result) { return std::apply(connect, resolve_result); }) |
         switch_on_next( )

         // Perform a websocket handshake
         | map([host, port = std::to_string(port)](const connect_result_t& connect_result) {
             return websocket_handshake(std::get<0>(connect_result), host, port);
           }) |
         switch_on_next( )

         // Now we're talking (literally)
         | map([](std::shared_ptr<state_type> state) { return create_websocket_observable(state); }) |
         switch_on_next( )

         // Make sure to disconnect in case of unsubscribe
         | lift<ServicePacket>([state](rxcpp::subscriber<ServicePacket> subscriber) {
             subscriber.get_subscription( ).add(
               [state, subscriber] { disconnect_websocket(std::move(state), subscriber); });
             return subscriber;
           })

         | tap([](auto p) { std::cout << "PACKET " << p.get_payload( ) << '\n'; },
               [](auto err) { std::cout << "ERROR\n"; }) |
         as_dynamic( );
}
}
