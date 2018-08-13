#include <iostream>
#include <trawler/cli/parse-configuration.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML { // NOLINT

std::ostream&
operator<<(std::ostream& ost, const trawler::config::websocket_client_service_t& svc)
{
  ost << "{ name: " << svc.name << ", service: " << svc.service << " }";
  return ost;
}

template<>
struct convert<trawler::config::websocket_client_service_t>
{
  static bool decode(const Node& node, trawler::config::websocket_client_service_t& svc)
  {
    svc.name = node["name"].as<std::string>( );
    svc.service = node["service"].as<std::string>( );
    svc.host = node["host"].as<std::string>( );
    svc.port = node["port"].as<unsigned short>( );
    svc.target = node["target"].as<std::string>( );
    svc.ssl = node["ssl"].as<bool>( );
    return true;
  }
};

template<>
struct convert<trawler::config::pipeline_t>
{
  static bool decode(const Node& node, trawler::config::pipeline_t& pipe)
  {
    pipe.name = node["name"].as<std::string>( );
    pipe.source = node["source"].as<std::string>( );
    pipe.event = node["event"].as<std::string>( );
    return true;
  }
};

template<>
struct convert<trawler::configuration_t>
{
  static bool decode(const Node& node, trawler::configuration_t& config)
  {
    if (!node.IsMap( )) {
      return false;
    }

    for (const auto& svc : node["services"]) {
      if (svc.IsMap( ) && svc["service"].as<std::string>( ) == "websocket-client") {
        config.services.emplace_back(svc.as<trawler::config::websocket_client_service_t>( ));
      }
    }

    for (const auto& pipe : node["pipelines"]) {
      if (pipe.IsMap( )) {
        config.pipelines.emplace_back(pipe.as<trawler::config::pipeline_t>( ));
      }
    }

    return true;
  }
};
}

namespace trawler {

configuration_t
parse_configuration(const std::string& configuration)
{
  return YAML::Load(configuration).as<configuration_t>( );
}
}
