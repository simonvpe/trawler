#pragma once
#include <algorithm>
#include <boost/asio/io_context.hpp>
#include <mutex>
#include <thread>
#include <trawler/logging/logger.hpp>
#include <vector>

namespace trawler {
class ServiceContext
{
  std::size_t nof_session_threads;
  std::size_t nof_service_threads;
  Logger logger;
  boost::asio::io_context session_context;
  boost::asio::io_context service_context;
  std::vector<std::thread> threads;
  mutable std::recursive_mutex mutex;

public:
  explicit ServiceContext(std::size_t nof_session_threads, std::size_t nof_service_threads, Logger logger)
    : nof_session_threads{ nof_session_threads }
    , nof_service_threads{ nof_service_threads }
    , logger{ std::move(logger) }
  {
    this->logger.debug("ServiceContext()");
  }

  ~ServiceContext( )
  {
    logger.debug("~ServiceContext()");
    join( );
  }

  ServiceContext(const ServiceContext&) = delete;
  ServiceContext(ServiceContext&&) = delete;
  ServiceContext& operator=(const ServiceContext&) = delete;
  ServiceContext& operator=(ServiceContext&&) = delete;

  void run( )
  {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    logger.debug("ServiceContext::run()");
    if (!running( )) {
      for (auto i = 0U; i < nof_session_threads; ++i) {
        threads.emplace_back([&] { session_context.run( ); });
      }
      for (auto i = 0U; i < nof_service_threads; ++i) {
        threads.emplace_back([&] { service_context.run( ); });
      }
    }
  }

  bool running( ) const
  {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    return !threads.empty( );
  }

  void join( )
  {
    std::lock_guard<std::recursive_mutex> guard(mutex);
    if (running( )) {
      logger.debug("ServiceContext::join()");
      session_context.stop( );
      service_context.stop( );
      for (auto& thread : threads) {
        if (thread.joinable( )) {
          thread.join( );
        }
      }
      threads.clear( );
    }
  }

  boost::asio::io_context& get_session_context( ) { return session_context; }

  boost::asio::io_context& get_service_context( ) { return service_context; }
};

inline std::shared_ptr<ServiceContext>
make_service_context(std::size_t nof_session_threads = 1,
                     std::size_t nof_service_threads = 1,
                     Logger logger = { "service-context" })
{
  return std::make_shared<ServiceContext>(nof_session_threads, nof_service_threads, std::move(logger));
}
}
