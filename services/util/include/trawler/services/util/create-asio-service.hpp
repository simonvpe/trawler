#pragma once
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <rxcpp/rx.hpp>
#include <trawler/services/service-packet.hpp>

namespace trawler {

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

enum class EStatus
{
  IDLE,
  SUBSCRIBED,
  ERROR,
  COMPLETED
};

template<typename Session>
struct lifetime_type
{
  using session_type = Session;
  using strand_type = asio::strand<asio::io_context::executor_type>;
  using status_type = EStatus;
  using endpoint_type = tcp::endpoint;
  using context_type = ServiceContext;

  std::shared_ptr<session_type> session;
  strand_type strand;
  status_type status;
  std::shared_ptr<context_type> context;

  lifetime_type(decltype(session) session, decltype(strand) strand, decltype(status) status, decltype(context) context)
    : session{ std::move(session) }
    , strand{ std::move(strand) }
    , status{ status }
    , context{ std::move(context) }
  {}
};

template<typename Session>
rxcpp::observable<ServicePacket>
create_asio_service(std::shared_ptr<lifetime_type<Session>> lifetime)
{
  auto subscribe = [lifetime](auto subscriber) {
    auto on_next = [lifetime, subscriber](ServicePacket data) {
      if (lifetime->status == EStatus::IDLE || lifetime->status == EStatus::SUBSCRIBED) {
        lifetime->status = EStatus::SUBSCRIBED;
        subscriber.on_next(std::move(data));
      }
    };

    auto on_error = [lifetime, subscriber](std::exception_ptr e) {
      if (lifetime->status == EStatus::IDLE || lifetime->status == EStatus::SUBSCRIBED) {
        lifetime->status = EStatus::ERROR;
        subscriber.on_error(std::move(e));
      }
    };

    auto on_closed = [lifetime, subscriber] {
      if (lifetime->status == EStatus::IDLE || lifetime->status == EStatus::SUBSCRIBED) {
        lifetime->status = EStatus::COMPLETED;
        subscriber.on_completed( );
      }
    };

    lifetime->session->run({ asio::bind_executor(lifetime->strand, std::move(on_next)),
                             asio::bind_executor(lifetime->strand, std::move(on_error)),
                             asio::bind_executor(lifetime->strand, std::move(on_closed)) });

    lifetime->context->run( );
  };

  auto unsubscribe = [lifetime = std::move(lifetime)](auto subscriber) {
    subscriber.get_subscription( ).add([lifetime] {
      lifetime->session->disconnect( );
      lifetime->session.reset( );
    });
    return subscriber;
  };

  return rxcpp::observable<>::create<ServicePacket>(subscribe) | rxcpp::operators::lift<ServicePacket>(unsubscribe) |
         rxcpp::operators::as_dynamic( );
}
}
