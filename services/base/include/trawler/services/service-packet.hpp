#pragma once
#include <functional>
#include <nlohmann/json.hpp>
#include <variant>

namespace trawler {

class ServicePacket
{
public:
  enum class EStatus
  {
    CONNECTED,
    DISCONNECTED,
    DATA_TRANSMISSION
  };
  using payload_t = std::variant<std::monostate, std::string, nlohmann::json>;
  using on_reply_t = std::function<void(std::string)>;

private:
  EStatus status;
  payload_t payload;
  on_reply_t on_reply;

public:
  bool reply(std::string reply_payload) const
  {
    if (on_reply) {
      on_reply(std::move(reply_payload));
      return true;
    }
    return false;
  }

  template<typename T>
  T get_payload_as( ) const;

  const payload_t& get_payload( ) const { return payload; }

  EStatus get_status( ) const { return status; }

  explicit ServicePacket(EStatus status)
    : status{ status }
  {}

  ServicePacket(EStatus status, payload_t payload)
    : status{ status }
    , payload{ std::move(payload) }
  {}

  ServicePacket(EStatus status, payload_t payload, on_reply_t on_reply)
    : status{ status }
    , payload{ std::move(payload) }
    , on_reply{ std::move(on_reply) }
  {}

  ServicePacket with_payload(payload_t payload) const
  {
    return ServicePacket{ this->status, std::move(payload), this->on_reply };
  }
};

template<>
inline std::string
ServicePacket::get_payload_as<std::string>( ) const
{
  if (std::holds_alternative<std::string>(payload)) {
    return std::get<std::string>(payload);
  }
  if (std::holds_alternative<nlohmann::json>(payload)) {
    return std::get<nlohmann::json>(payload).dump( );
  }
  return "";
}

template<>
inline nlohmann::json
ServicePacket::get_payload_as<nlohmann::json>( ) const
{
  if (std::holds_alternative<std::string>(payload)) {
    return nlohmann::json::parse(std::get<std::string>(payload));
  }
  if (std::holds_alternative<nlohmann::json>(payload)) {
    return std::get<nlohmann::json>(payload);
  }
  return nlohmann::json{};
}
}
