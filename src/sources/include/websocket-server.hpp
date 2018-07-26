#pragma once
//******************************************************************************

#include <rxcpp/rx.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>


using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>

//******************************************************************************

// Report a failure
void
fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Echoes back all received WebSocket messages
class session : public std::enable_shared_from_this<session>
{
  websocket::stream<tcp::socket> ws_;
  boost::asio::strand<boost::asio::io_context::executor_type> strand_;
  boost::beast::multi_buffer buffer_;

public:
  // Take ownership of the socket
  explicit
  session(tcp::socket socket)
    : ws_(std::move(socket))
    , strand_(ws_.get_executor())
  {
  }

  // Start the asynchronous operation
  void
  run()
  {
    // Accept the websocket handshake
    auto fn = std::bind(&session::on_accept, shared_from_this(), std::placeholders::_1);
    auto executor = boost::asio::bind_executor(strand_, fn);
    ws_.async_accept(executor);
  }

  void
  on_accept(boost::system::error_code ec)
  {
    if(ec)
      return fail(ec, "accept");

    // Read a message
    do_read();
  }

  void
  do_read()
  {
    // Read a message into our buffer
    using namespace std::placeholders;
    auto fn = std::bind(&session::on_read, shared_from_this(), _1, _2);
    auto executor = boost::asio::bind_executor(strand_, fn);
    ws_.async_read(buffer_, executor);
  }

  void
  on_read(
	  boost::system::error_code ec,
	  std::size_t bytes_transferred)
  {
    boost::ignore_unused(bytes_transferred);

    // This indicates that the session was closed
    if(ec == websocket::error::closed)
      return;

    if(ec)
      fail(ec, "read");

    // Echo the message
    ws_.text(ws_.got_text());
    using namespace std::placeholders;
    auto fn = std::bind(&session::on_write, shared_from_this(), _1, _2);
    auto executor = boost::asio::bind_executor(strand_, fn);
    ws_.async_write(buffer_.data(), fn);
  }

  void
  on_write(
	   boost::system::error_code ec,
	   std::size_t bytes_transferred)
  {
    boost::ignore_unused(bytes_transferred);

    if(ec)
      return fail(ec, "write");

    // Clear the buffer
    buffer_.consume(buffer_.size());

    // Do another read
    do_read();
  }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener>
{
  tcp::acceptor acceptor_;
  tcp::socket socket_;

public:
  listener(
	   boost::asio::io_context& ioc,
	   tcp::endpoint endpoint)
    : acceptor_(ioc)
    , socket_(ioc)
  {
    boost::system::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec)
      {
	fail(ec, "open");
	return;
      }

    // Allow address reuse
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if(ec)
      {
	fail(ec, "set_option");
	return;
      }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec)
      {
	fail(ec, "bind");
	return;
      }

    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if(ec)
      {
	fail(ec, "listen");
	return;
      }
  }

  // Start accepting incoming connections
  void
  run()
  {
    if(! acceptor_.is_open())
      return;
    do_accept();
  }

  void
  do_accept()
  {
    using namespace std::placeholders;
    auto fn = std::bind(&listener::on_accept, shared_from_this(), _1);
    acceptor_.async_accept(socket_, fn);
  }

  void
  on_accept(boost::system::error_code ec)
  {
    if(ec)
      {
	fail(ec, "accept");
      }
    else
      {
	// Create the session and run it
	std::make_shared<session>(std::move(socket_))->run();
      }

    // Accept another connection
    do_accept();
  }
};

namespace trawler {

  auto websocket_server(unsigned long port) {
    
    return 1;
  }
}
