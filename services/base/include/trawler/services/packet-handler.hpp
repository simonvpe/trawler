#pragma once
#include <functional>
#include <trawler/services/service-packet.hpp>

namespace trawler {

class PacketHandler
{
public:
  using on_data_t = std::function<void(ServicePacket)>;
  using on_error_t = std::function<void(std::exception_ptr)>;
  using on_closed_t = std::function<void( )>;

private:
  on_data_t on_data;
  on_error_t on_error;
  on_closed_t on_closed;

public:
  bool call_on_data(ServicePacket packet)
  {
    if (on_data) {
      on_data(std::move(packet));
      return true;
    }
    return false;
  }

  bool call_on_error(std::exception_ptr e)
  {
    if (on_error) {
      on_error(std::move(e));
      return true;
    }
    return false;
  }

  bool call_on_closed( )
  {
    if (on_closed) {
      on_closed( );
      return true;
    }
    return false;
  }

  PacketHandler( ) = default;

  PacketHandler(on_data_t on_data, on_error_t on_error, on_closed_t on_closed)
    : on_data{ std::move(on_data) }
    , on_error{ std::move(on_error) }
    , on_closed{ std::move(on_closed) }
  {}
};
}
