#pragma once
#include <string>
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

struct pipeline_t
{
  std::string name = "";
  std::string pipeline = "";
  std::string source = "";
  std::string event = "";
};

struct endpoint_t
{
  std::string name = "";
  std::string source = "";
  std::string event = "";
  std::string data;
};
}

struct configuration_t
{
  using service_t = std::variant<std::monostate, config::websocket_client_service_t>;
  using pipeline_t = std::variant<std::monostate>;
  using endpoint_t = config::endpoint_t;

  std::vector<service_t> services = {};
  std::vector<pipeline_t> pipelines = {};
  std::vector<endpoint_t> endpoints = {};
};
}
