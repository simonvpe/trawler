#pragma once
#include <algorithm>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/asio/io_context.hpp>
#include <mutex>
#include <thread>
#include <vector>

namespace trawler {
class ServiceContext
{
  std::size_t nof_session_threads;
  std::size_t nof_service_threads;
  boost::asio::io_context session_context;
  boost::asio::io_context service_context;
  std::vector<std::thread> threads;
  mutable std::recursive_mutex mutex;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> session_work_guard;
  boost::asio::executor_work_guard<boost::asio::io_context::executor_type> service_work_guard;

public:
  explicit ServiceContext(std::size_t nof_session_threads, std::size_t nof_service_threads)
    : nof_session_threads{ nof_session_threads }
    , nof_service_threads{ nof_service_threads }
    , session_work_guard{ session_context.get_executor( ) }
    , service_work_guard{ service_context.get_executor( ) }
  {
    for (auto i = 0U; i < nof_session_threads; ++i) {
      threads.emplace_back([&] { session_context.run( ); });
    }
    for (auto i = 0U; i < nof_service_threads; ++i) {
      threads.emplace_back([&] { service_context.run( ); });
    }
  }

  ~ServiceContext( )
  {
    session_work_guard.reset( );
    service_work_guard.reset( );
    session_context.stop( );
    service_context.stop( );
    for (auto& thread : threads) {
      if (thread.joinable( )) {
        thread.join( );
      }
    }
  }

  ServiceContext(const ServiceContext&) = delete;
  ServiceContext(ServiceContext&&) = delete;
  ServiceContext& operator=(const ServiceContext&) = delete;
  ServiceContext& operator=(ServiceContext&&) = delete;

  boost::asio::io_context& get_session_context( ) { return session_context; }

  boost::asio::io_context& get_service_context( ) { return service_context; }
};

inline std::shared_ptr<ServiceContext>
make_service_context(std::size_t nof_session_threads = 1,
                     std::size_t nof_service_threads = 1)
{
  return std::make_shared<ServiceContext>(nof_session_threads, nof_service_threads);
}
}
