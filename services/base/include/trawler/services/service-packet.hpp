#pragma once
#include <functional>

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
  using payload_t = std::string;
  using on_reply_t = std::function<void(payload_t)>;

private:
  EStatus status;
  payload_t payload;
  on_reply_t on_reply;

public:
  bool call_on_reply(payload_t reply_payload)
  {
    if (on_reply) {
      on_reply(std::move(reply_payload));
      return true;
    }
    return false;
  }

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
};
}
