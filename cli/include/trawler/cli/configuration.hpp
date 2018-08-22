#pragma once
#include <optional>
#include <string>
#include <trawler/services/service-packet.hpp>
#include <variant>
#include <vector>

namespace trawler {

namespace config {

struct service_t
{
  std::string name = "";
  std::string service = "";
};

struct websocket_client_service_t : public service_t
{
  std::string target = "";
  std::string host = "";
  unsigned short port = 0;
  bool ssl = false;
};

struct http_server_service_t : public service_t
{
  std::string host = "";
  unsigned short port = 0;
};

struct pipeline_t
{
  std::string name = "";
  std::string pipeline = "";
  std::string source = "";
  std::vector<ServicePacket::EStatus> event = { ServicePacket::EStatus::DATA_TRANSMISSION };
};

struct inja_pipeline_t : public pipeline_t
{
  std::string tmplate = "";
};

struct jq_pipeline_t : public pipeline_t
{
  std::string script = "";
};

struct emit_pipeline_t : public pipeline_t
{
  std::string data = "";
};

struct buffer_pipeline_t : public pipeline_t
{
  std::string trigger_source = "";
  std::vector<ServicePacket::EStatus> trigger_event = { ServicePacket::EStatus::DATA_TRANSMISSION };
};

struct endpoint_t
{
  std::string name = "";
  std::string source = "";
  std::string event = "";
  std::optional<std::string> data;
};
}

struct configuration_t
{
  using service_t = std::variant<config::websocket_client_service_t, config::http_server_service_t>;
  using pipeline_t =
    std::variant<config::inja_pipeline_t, config::jq_pipeline_t, config::buffer_pipeline_t, config::emit_pipeline_t>;
  using endpoint_t = config::endpoint_t;

  std::vector<service_t> services = {};
  std::vector<pipeline_t> pipelines = {};
  std::vector<endpoint_t> endpoints = {};
};
}
