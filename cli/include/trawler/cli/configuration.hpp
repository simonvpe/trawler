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
  std::string source = "";
  std::string event = "";
};
};

struct configuration_t
{
  using service_t = std::variant<std::monostate, config::websocket_client_service_t>;
  using pipeline_t = config::pipeline_t;
  std::vector<service_t> services = {};
  std::vector<pipeline_t> pipelines = {};
};
}
