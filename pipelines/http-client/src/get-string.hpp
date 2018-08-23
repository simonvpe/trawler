#pragma once
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace trawler {

inline std::string
get_string(const nlohmann::json& node, const std::string& field)
{
  if (node.find(field) == cend(node) || node[field].type( ) != nlohmann::json::value_t::string) {
    throw std::runtime_error{ "key \"" + field + "\" required" };
  }
  return node[field].get<std::string>( );
}
}
