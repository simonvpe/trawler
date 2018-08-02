#pragma once
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <fmt/format.h>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/packet-handler.hpp>
#include <utility>

//------------------------------------------------------------------------------

namespace trawler {
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

template<typename S>
class ConnectionPool : public std::enable_shared_from_this<ConnectionPool<S>>
{
  std::set<std::shared_ptr<S>> sessions;
  asio::strand<asio::io_context::executor_type> strand;
  Logger logger;

public:
  ConnectionPool(asio::io_context& context, Logger logger)
    : strand{ context.get_executor( ) }
    , logger{ std::move(logger) }
  {}

  void run(std::shared_ptr<S> session)
  {
    sessions.insert(session);

    auto me = this->shared_from_this( );

    asio::bind_executor(strand, [me, session] {
      auto eraser = asio::bind_executor(me->strand, [me](std::shared_ptr<S> s) {
        me->logger.debug("Removing session from pool");
        me->sessions.erase(s);
      });
      me->logger.debug("Adding session to pool");
      session->run(eraser);
    })( );
  }

  void disconnect( )
  {
    auto me = this->shared_from_this( );
    auto fn = [me] {
      me->logger.debug("ConnectionPool::disconnect()");
      std::for_each(begin(me->sessions), end(me->sessions), [=](auto& session) { session->disconnect( ); });
    };
    asio::bind_executor(strand, fn)( );
  }

  template<typename T>
  void write(T data)
  {
    auto fn = [data](std::shared_ptr<S> session) { session->write(std::move(data)); };
    std::for_each(begin(sessions), end(sessions), fn);
  }
};

template<typename Session>
class TcpListener : public std::enable_shared_from_this<TcpListener<Session>>
{
  using session_type = Session;

  tcp::endpoint endpoint;
  tcp::acceptor acceptor;
  tcp::socket socket;
  PacketHandler packet_handler;
  std::shared_ptr<ConnectionPool<session_type>> connection_pool;
  Logger logger;
  asio::strand<asio::io_context::executor_type> strand;

  void _fail_on_error(boost::system::error_code ec)
  {
    try {
      if (ec) {
        logger.critical(ec.message( ));
        throw std::runtime_error(ec.message( ));
      }
    } catch (...) {
      std::rethrow_exception(std::current_exception( ));
    }
  }

  void _do_accept( )
  {
    auto me = this->shared_from_this( );
    auto fn = [me](boost::system::error_code ec) {
      if (ec == boost::system::errc::operation_canceled) {
        return;
      }

      me->_fail_on_error(ec);

      const auto address = me->socket.remote_endpoint( ).address( );
      const auto message = format(fmt("Client connected from {}"), address.to_string( ));
      me->logger.info(message);

      auto session = std::make_shared<session_type>(std::move(me->socket), me->packet_handler, me->logger);
      me->connection_pool->run(session);
      me->_do_accept( );
    };
    acceptor.async_accept(socket, std::move(fn));
  }

public:
  TcpListener(asio::io_context& context, tcp::endpoint endpoint, Logger logger)
    : endpoint{ endpoint }
    , acceptor{ context }
    , socket{ context }
    , connection_pool{ std::make_shared<ConnectionPool<session_type>>(context, logger) }
    , logger{ std::move(logger) }
    , strand{ context.get_executor( ) }
  {
    this->logger.debug("TcpListener()");
    boost::system::error_code ec;

    acceptor.open(endpoint.protocol( ), ec);
    _fail_on_error(ec);

    acceptor.set_option(asio::socket_base::reuse_address(true), ec);
    _fail_on_error(ec);

    acceptor.bind(endpoint, ec);
    _fail_on_error(ec);

    acceptor.listen(asio::socket_base::max_listen_connections, ec);
    _fail_on_error(ec);
  }

  ~TcpListener( ) { logger.debug("~TcpListener()"); }

  TcpListener(const TcpListener&) = delete;
  TcpListener(TcpListener&&) = delete;
  TcpListener& operator=(const TcpListener&) = delete;
  TcpListener& operator=(TcpListener&&) = delete;

  void run(PacketHandler packet_handler)
  {
    if (!acceptor.is_open( )) {
      return;
    }
    this->packet_handler = std::move(packet_handler);
    _do_accept( );
    const auto message = format(fmt("Listening on port {}"), endpoint.port( ));
    logger.info(message);
  }

  void disconnect( )
  {
    connection_pool->disconnect( );
    auto me = this->shared_from_this( );
    asio::bind_executor(strand, [me] {
      me->logger.debug("TcpListener::disconnect()");
      if (me->acceptor.is_open( )) {
        me->acceptor.close( );
      }
    })( );
  }

  template<typename T>
  auto write(T data)
  {
    auto me = this->shared_from_this( );
    asio::bind_executor(strand, [me, payload = std::move(data)] {
      me->logger.debug("TcpListener::write()");
      me->connection_pool->write(std::move(payload));
    })( );
  }
};
}
