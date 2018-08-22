#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <nlohmann/json.hpp>
#include <trawler/services/http-server/http-server.hpp>
#include <trawler/services/tcp-common/make-tcp-acceptor.hpp>
#include <trawler/services/tcp-common/make-tcp-listener.hpp>

namespace trawler {

template<typename DoRead>
void
run_http_event_loop(DoRead do_read)
{
  namespace http = boost::beast::http;
  using json = nlohmann::json;

  do_read([=](auto ec, auto logger, auto request, auto buffer, auto on_next, auto on_error, auto on_completed) {
    using status_t = ServicePacket::EStatus;

    if (ec == http::error::end_of_stream || ec == boost::system::errc::operation_canceled ||
        ec == boost::asio::error::eof) {
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

    auto json_object = json{ { "method", request->method_string( ) },
                             { "target", request->target( ) },
                             { "body", request->body( ) } };

    for (const auto& header : request->base( )) {
      json_object["headers"][std::string{ header.name_string( ) }] = std::string{ header.value( ) };
    }

    const auto json_string = json_object.dump( );
    logger.debug(json_string);
    on_next(status_t::DATA_TRANSMISSION, json_string);

    run_http_event_loop(do_read);
  });
}

auto
make_http_event_loop(const std::shared_ptr<ServiceContext>& context, const Logger& logger)
{
  namespace http = boost::beast::http;

  using socket_t = boost::asio::ip::tcp::socket;
  using socket_tp = std::shared_ptr<socket_t>;
  using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;
  using error_t = boost::system::error_code;

  return [=](const socket_tp& socket) {
    using result_t = ServicePacket;

    auto session_strand = std::make_shared<strand_t>(context->get_session_context( ).get_executor( ));
    auto service_strand = std::make_shared<strand_t>(context->get_service_context( ).get_executor( ));

    auto on_subscribe = [=](auto subscriber) {
      using status_t = ServicePacket::EStatus;
      using data_t = const std::string&;

      auto buffer = std::make_shared<boost::beast::flat_buffer>( );
      auto request = std::make_shared<http::request<http::string_body>>( );

      auto on_write = [=](data_t data) {
        auto response = http::response<http::string_body>{ http::status::ok, request->version( ) };
        response.set(http::field::server, "1.0");
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request->keep_alive( ));
        response.body( ) = data;
        response.prepare_payload( );
        using message_type = http::message<false, decltype(response)::body_type, decltype(response)::fields_type>;
        auto message = std::make_shared<message_type>(std::move(response));
        auto fn = [=] {
          auto cb = [message, socket, session_strand](error_t, std::size_t) {};
          http::async_write(*socket, *message, boost::asio::bind_executor(*session_strand, cb));
        };
        boost::asio::bind_executor(*service_strand, fn)( );
      };

      auto on_next = [=](status_t status, data_t data = "") {
        auto fn = boost::asio::bind_executor(*service_strand, [=] {
          subscriber.on_next(ServicePacket{ status, { data }, on_write });
        });
        fn( );
      };

      auto on_error = [=](std::exception_ptr e) {
        auto fn = boost::asio::bind_executor(*service_strand, [=] { subscriber.on_error(e); });
        fn( );
      };

      auto on_completed = [=]( ) {
        auto fn = boost::asio::bind_executor(*service_strand, [=]( ) { subscriber.on_completed( ); });
        fn( );
      };

      on_next(status_t::CONNECTED);

      auto do_read = [=](auto on_read_impl) {
        auto read_callback = [=](error_t ec, std::size_t bytes_transferred) {
          on_read_impl(ec, logger, request, buffer, on_next, on_error, on_completed);
        };
        http::async_read(
          *socket, *buffer, *request, boost::asio::bind_executor(*session_strand, std::move(read_callback)));
      };
      run_http_event_loop(std::move(do_read));
    };

    return rxcpp::observable<>::create<result_t>(std::move(on_subscribe));
  };
}

rxcpp::observable<ServicePacket>
create_http_server(const std::shared_ptr<ServiceContext>& context,
                   const std::string& host,
                   unsigned short port,
                   const Logger& logger)
{
  auto tcp_listener = make_tcp_listener(context, logger, host, port);
  auto tcp_acceptor = make_tcp_acceptor(context, logger);
  auto http_event_loop = make_http_event_loop(context, logger);

  return tcp_listener( ).flat_map(std::move(tcp_acceptor)).flat_map(std::move(http_event_loop));
}
}
