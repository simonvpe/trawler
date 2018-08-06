#pragma once

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/websocket.hpp>
#include <trawler/logging/logger.hpp>
#include <rxcpp/rx.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/websocket-common/make-runtime-error.hpp>

namespace trawler {

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = asio::ip::tcp;

/*******************************************************************************
 * run_websocket_event_loop
 ******************************************************************************/
template<typename DoRead>
inline void
run_websocket_event_loop(DoRead do_read)
{
  do_read([=](auto ec, auto logger, auto buffer, auto on_next, auto on_error, auto on_completed) {
    using status_t = ServicePacket::EStatus;

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
template<typename Stream>
inline auto
make_websocket_event_loop(const std::shared_ptr<ServiceContext>& context, const Logger& logger)
{
  using stream_t = Stream;
  using stream_tp = std::shared_ptr<stream_t>;
  using strand_t = asio::strand<asio::io_context::executor_type>;
  using error_t = boost::system::error_code;
  
  return [=](const stream_tp& stream) {
    using result_t = ServicePacket;

    auto buffer = std::make_shared<beast::multi_buffer>( );
    auto session_strand = std::make_shared<strand_t>(context->get_session_context( ).get_executor( ));
    auto service_strand = std::make_shared<strand_t>(context->get_service_context( ).get_executor( ));

    auto on_subscribe = [=](auto subscriber) {
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
}
