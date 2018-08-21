#include <iostream>
#include <trawler/cli/parse-configuration.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML { // NOLINT

/*******************************************************************************
 * convert websocket_client_service_t
 *******************************************************************************/
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

/*******************************************************************************
 * convert http_server_service_t
 *******************************************************************************/
template<>
struct convert<trawler::config::http_server_service_t>
{
  static bool decode(const Node& node, trawler::config::http_server_service_t& svc)
  {
    svc.name = node["name"].as<std::string>( );
    svc.service = node["service"].as<std::string>( );
    svc.host = node["host"].as<std::string>( );
    svc.port = node["port"].as<unsigned short>( );
    return true;
  }
};

/*******************************************************************************
 * convert inja_pipeline_t
 *******************************************************************************/
template<>
struct convert<trawler::config::inja_pipeline_t>
{
  static bool decode(const Node& node, trawler::config::inja_pipeline_t& pipe)
  {
    pipe.name = node["name"].as<std::string>( );
    pipe.pipeline = node["pipeline"].as<std::string>( );
    pipe.source = node["source"].as<std::string>( );
    pipe.event = node["event"].as<std::string>( );
    pipe.tmplate = node["template"].as<std::string>( );
    return true;
  }
};

/*******************************************************************************
 * convert jq_pipeline_t
 *******************************************************************************/
template<>
struct convert<trawler::config::jq_pipeline_t>
{
  static bool decode(const Node& node, trawler::config::jq_pipeline_t& pipe)
  {
    pipe.name = node["name"].as<std::string>( );
    pipe.pipeline = node["pipeline"].as<std::string>( );
    pipe.source = node["source"].as<std::string>( );
    pipe.event = node["event"].as<std::string>( );
    pipe.script = node["script"].as<std::string>( );
    return true;
  }
};

/*******************************************************************************
 * convert endpoint_t
 *******************************************************************************/
template<>
struct convert<trawler::config::endpoint_t>
{
  static bool decode(const Node& node, trawler::config::endpoint_t& pipe)
  {
    pipe.name = node["name"].as<std::string>( );
    pipe.source = node["source"].as<std::string>( );
    pipe.event = node["event"].as<std::string>( );

    if (node["data"]) {
      pipe.data = node["data"].as<std::string>( );
    }
    return true;
  }
};

/*******************************************************************************
 * convert configuration_t
 *******************************************************************************/
template<>
struct convert<trawler::configuration_t>
{
  static bool decode(const Node& node, trawler::configuration_t& config)
  {
    if (!node.IsMap( )) {
      return false;
    }

    decode_services(node, config);
    decode_pipelines(node, config);
    decode_endpoints(node, config);

    return true;
  }

  static void decode_services(const Node& node, trawler::configuration_t& config)
  {
    for (const auto& svc : node["services"]) {
      if (svc.IsMap( ) && svc["service"].as<std::string>( ) == "websocket-client") {
        config.services.emplace_back(svc.as<trawler::config::websocket_client_service_t>( ));
      }
      if (svc.IsMap( ) && svc["service"].as<std::string>( ) == "http-server") {
        config.services.emplace_back(svc.as<trawler::config::http_server_service_t>( ));
      }
    }
  }

  static void decode_pipelines(const Node& node, trawler::configuration_t& config)
  {
    for (const auto& pipe : node["pipelines"]) {
      if (pipe.IsMap( ) && pipe["pipeline"].as<std::string>( ) == "inja") {
        config.pipelines.emplace_back(pipe.as<trawler::config::inja_pipeline_t>( ));
      }
      if (pipe.IsMap( ) && pipe["pipeline"].as<std::string>( ) == "jq") {
        config.pipelines.emplace_back(pipe.as<trawler::config::jq_pipeline_t>( ));
      }
    }
  }

  static void decode_endpoints(const Node& node, trawler::configuration_t& config)
  {
    for (const auto& endp : node["endpoints"]) {
      if (endp.IsMap( )) {
        config.endpoints.emplace_back(endp.as<trawler::config::endpoint_t>( ));
      }
    }
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
