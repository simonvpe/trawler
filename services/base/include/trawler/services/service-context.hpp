#pragma once
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <thread>
#include <vector>

namespace trawler {
class ServiceContext
{

  struct context_instance
  {
    boost::asio::io_context context;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> guard;
    std::vector<std::thread> threads;

    explicit context_instance(std::size_t nof_threads)
      : guard{ context.get_executor( ) }
    {
      for (auto i = 0U; i < nof_threads; ++i) {
        threads.emplace_back([&] { context.run( ); });
      }
    }

    ~context_instance( )
    {
      guard.reset( );
      context.stop( );
      for (auto& thread : threads) {
        if (thread.joinable( )) {
          thread.join( );
        }
      }
    }

    context_instance(const context_instance&) = delete;
    context_instance(context_instance&&) = delete;
    context_instance& operator=(const context_instance&) = delete;
    context_instance& operator=(context_instance&&) = delete;
  };

  context_instance session_context;
  context_instance service_context;

public:
  ServiceContext(std::size_t nof_session_threads, std::size_t nof_service_threads)
    : session_context{ nof_session_threads }
    , service_context{ nof_service_threads }
  {}

  ServiceContext(const ServiceContext&) = delete;
  ServiceContext(ServiceContext&&) = delete;
  ServiceContext& operator=(const ServiceContext&) = delete;
  ServiceContext& operator=(ServiceContext&&) = delete;
  ~ServiceContext( ) = default;

  boost::asio::io_context& get_session_context( ) { return session_context.context; }
  boost::asio::io_context& get_service_context( ) { return service_context.context; }
};

inline std::shared_ptr<ServiceContext>
make_service_context(std::size_t nof_session_threads = 1, std::size_t nof_service_threads = 1)
{
  return std::make_shared<ServiceContext>(nof_session_threads, nof_service_threads);
}
}
