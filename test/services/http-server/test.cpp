#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <trawler/services/http-server/http-server.hpp>

#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <trawler/services/service-packet.hpp>
#include <trawler/services/tcp-common/make-tcp-acceptor.hpp>
#include <trawler/services/tcp-common/make-tcp-listener.hpp>
namespace trawler {

auto
make_http_event_loop(const std::shared_ptr<ServiceContext>& context, const Logger& logger)
{
  using socket_t = boost::asio::ip::tcp::socket;
  using socket_tp = std::shared_ptr<socket_t>;
  using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;
  using error_t = boost::system::error_code;

  return [=](const socket_tp& socket) {
    using result_t = ServicePacket;
    return rxcpp::observable<>::create<result_t>([](auto subscriber) {});
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

SCENARIO("dummy http-server")
{
  using namespace trawler;
  auto context = make_service_context( );
  create_http_server(context, "0.0.0.0", 5001, { "my-http-server" }).as_blocking( ).subscribe([](auto s) {
    std::cout << "PAYLOAD: " << s.get_payload( ) << "\n";
  });
}
