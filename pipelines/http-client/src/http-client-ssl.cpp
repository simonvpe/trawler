#include "get-string.hpp"
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
#include <trawler/services/tcp-common/load-root-certificates.hpp>

namespace trawler {

using tcp = boost::asio::ip::tcp;    // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

std::function<ServicePacket(ServicePacket)>
create_http_client_ssl_pipeline(const Logger& logger)
{
  return [=](const ServicePacket& service_packet) {
    const auto payload = service_packet.get_payload_as<nlohmann::json>( );
    logger.debug("Got (ssl) " + payload.dump( ));
    const auto host = get_string(payload, "host");
    const auto port = get_string(payload, "port");
    const auto target = get_string(payload, "target");
    const auto version = get_string(payload, "version") == "1.0" ? 10 : 11;

    // The io_context is required for all I/O
    boost::asio::io_context ioc;

    // The SSL context is required, and holds certificates
    ssl::context ctx{ ssl::context::sslv23_client };

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // These objects perform our I/O
    tcp::resolver resolver{ ioc };
    ssl::stream<tcp::socket> stream{ ioc, ctx };

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle( ), host.c_str( ))) {
      boost::system::error_code ec{ static_cast<int>(::ERR_get_error( )), boost::asio::error::get_ssl_category( ) };
      throw boost::system::system_error{ ec };
    }

    // Look up the domain name
    auto const results = resolver.resolve(host, port);

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream.next_layer( ), results.begin( ), results.end( ));

    // Perform the SSL handshake
    stream.handshake(ssl::stream_base::client);

    // Set up an HTTP GET request message
    http::request<http::string_body> req{ http::verb::get, target.c_str( ), version };
    req.set(http::field::host, host);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    // Send the HTTP request to the remote host
    http::write(stream, req);

    // This buffer is used for reading and must be persisted
    boost::beast::flat_buffer buffer;

    // Declare a container to hold the response
    http::response<http::dynamic_body> res;

    // Receive the HTTP response
    http::read(stream, buffer, res);

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
    stream.shutdown(ec);
    if (ec == boost::asio::error::eof || ec == boost::asio::ssl::error::stream_truncated) {
      // Rationale:
      // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
      ec.assign(0, ec.category( ));
    }
    if (ec)
      throw boost::system::system_error{ ec };

    // If we get here then the connection is closed gracefully
    return service_packet.with_payload(std::move(json));
  };
}
}
