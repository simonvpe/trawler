#pragma once
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <rxcpp/rx.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/make-runtime-error.hpp>
#include <trawler/services/service-context.hpp>

namespace trawler {
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
  using acceptor_t = boost::asio::ip::tcp::acceptor;
  using acceptor_tp = std::shared_ptr<acceptor_t>;
  using error_t = boost::system::error_code;
  using socket_t = boost::asio::ip::tcp::socket;
  using socket_tp = std::shared_ptr<socket_t>;
  using strand_t = boost::asio::strand<boost::asio::io_context::executor_type>;

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
}
