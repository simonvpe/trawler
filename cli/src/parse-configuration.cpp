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
 * get_events
 *******************************************************************************/
std::optional<std::vector<trawler::ServicePacket::EStatus>>
get_events(const Node& node, const std::string& property_name)
{
  auto parse_event = [](const std::string& event_str) {
    if (event_str == "data") {
      return trawler::ServicePacket::EStatus::DATA_TRANSMISSION;
    }
    if (event_str == "connected") {
      return trawler::ServicePacket::EStatus::CONNECTED;
    }
    if (event_str == "disconnected") {
      return trawler::ServicePacket::EStatus::DISCONNECTED;
    }
    throw std::runtime_error("Unknow event " + event_str);
  };

  if (node[property_name]) {
    std::vector<trawler::ServicePacket::EStatus> result{};

    if (node[property_name].IsSequence( )) {
      for (const auto& ev : node["event"]) {
        const auto event_str = ev.as<std::string>( );
        result.push_back(parse_event(event_str));
      }
    } else if (node[property_name].IsScalar( )) {
      const auto event_str = node[property_name].as<std::string>( );
      result.push_back(parse_event(event_str));
    } else {
      throw std::runtime_error{ "Bad event" };
    }

    return { std::move(result) };
  }
  return std::nullopt;
}

/*******************************************************************************
 * convert pipeline_t
 *******************************************************************************/
template<>
struct convert<trawler::config::pipeline_t>
{
  static bool decode(const Node& node, trawler::config::pipeline_t& pipe)
  {
    pipe.name = node["name"].as<std::string>( );
    pipe.pipeline = node["pipeline"].as<std::string>( );
    pipe.source = node["source"].as<std::string>( );

    if (auto events = get_events(node, "event")) {
      pipe.event = std::move(events.value( ));
    }
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
    convert<trawler::config::pipeline_t>::decode(node, pipe);
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
    convert<trawler::config::pipeline_t>::decode(node, pipe);
    pipe.script = node["script"].as<std::string>( );
    return true;
  }
};

/*******************************************************************************
 * convert buffer_pipeline_t
 *******************************************************************************/
template<>
struct convert<trawler::config::buffer_pipeline_t>
{
  static bool decode(const Node& node, trawler::config::buffer_pipeline_t& pipe)
  {
    convert<trawler::config::pipeline_t>::decode(node, pipe);
    pipe.trigger_source = node["trigger_source"].as<std::string>( );

    if (auto events = get_events(node, "trigger_event")) {
      pipe.trigger_event = std::move(events.value( ));
    }

    return true;
  }
};

/*******************************************************************************
 * convert emit_pipeline_t
 *******************************************************************************/
template<>
struct convert<trawler::config::emit_pipeline_t>
{
  static bool decode(const Node& node, trawler::config::emit_pipeline_t& pipe)
  {
    convert<trawler::config::pipeline_t>::decode(node, pipe);
    pipe.data = node["data"].as<std::string>( );
    return true;
  }
};

/*******************************************************************************
 * convert http_client_pipeline_t
 *******************************************************************************/
template<>
struct convert<trawler::config::http_client_pipeline_t>
{
  static bool decode(const Node& node, trawler::config::http_client_pipeline_t& pipe)
  {
    if (node["ssl"]) {
      pipe.ssl = node["ssl"].as<bool>( );
    }
    return convert<trawler::config::pipeline_t>::decode(node, pipe);
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
      } else if (pipe.IsMap( ) && pipe["pipeline"].as<std::string>( ) == "jq") {
        config.pipelines.emplace_back(pipe.as<trawler::config::jq_pipeline_t>( ));
      } else if (pipe.IsMap( ) && pipe["pipeline"].as<std::string>( ) == "buffer") {
        config.pipelines.emplace_back(pipe.as<trawler::config::buffer_pipeline_t>( ));
      } else if (pipe.IsMap( ) && pipe["pipeline"].as<std::string>( ) == "emit") {
        config.pipelines.emplace_back(pipe.as<trawler::config::emit_pipeline_t>( ));
      } else if (pipe.IsMap( ) && pipe["pipeline"].as<std::string>( ) == "http-client") {
        config.pipelines.emplace_back(pipe.as<trawler::config::http_client_pipeline_t>( ));
      }
    }
  }

  static void decode_endpoints(const Node& node, trawler::configuration_t& config)
  {
    config.endpoints = std::move(node["endpoints"].as<std::vector<trawler::configuration_t::endpoint_t>>( ));
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
