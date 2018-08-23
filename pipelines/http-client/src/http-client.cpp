#include <boost/algorithm/string/predicate.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <trawler/pipelines/http-client/http-client.hpp>

namespace trawler {

using tcp = boost::asio::ip::tcp;    // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

std::function<ServicePacket(ServicePacket)>
create_http_client_pipeline(const Logger& logger)
{
  return [=](const ServicePacket& service_packet) {
    const auto payload = service_packet.get_payload_as<nlohmann::json>( );
    const auto host = payload["host"].get<std::string>( );
    const auto port = payload["port"].get<std::string>( );
    const auto target = payload["target"].get<std::string>( );
    const auto version = payload["version"].get<std::string>( ) == "1.0" ? 10 : 11;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // These objects perform our I/O
    tcp::resolver resolver{ ioc };
    tcp::socket socket{ ioc };

    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(socket, results.begin( ), results.end( ));

    // Set up an HTTP GET request message
    http::request<http::string_body> req{ http::verb::get, target.c_str( ), version };
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(socket, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(socket, buffer, res);

    // Store the result
    nlohmann::json json;
    for (const auto& header : res.base( )) {
      json["headers"][std::string{ header.name_string( ) }] = std::string{ header.value( ) };
    }
    if (boost::starts_with(res[http::field::content_type], "application/json")) {
      json["body"] = nlohmann::json::parse(boost::beast::buffers_to_string(res.body( ).data( )));
    } else {
      json["body"] = boost::beast::buffers_to_string(res.body( ).data( ));
    }

    // Gracefully close the socket
    boost::system::error_code ec;
    socket.shutdown(tcp::socket::shutdown_both, ec);

    // not_connected happens sometimes
    // so don't bother reporting it.
    //
    if (ec && ec != boost::system::errc::not_connected)
      throw boost::system::system_error{ ec };

    // If we get here then the connection is closed gracefully
    return service_packet.with_payload(std::move(json));
  };
}
}
