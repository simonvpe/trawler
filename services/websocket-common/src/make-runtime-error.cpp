#include <trawler/services/websocket-common/make-runtime-error.hpp>

namespace trawler {
std::exception_ptr
make_runtime_error(const std::string& message)
{
  return std::make_exception_ptr(std::runtime_error{ message });
}

std::exception_ptr
make_runtime_error(const boost::system::error_code& e)
{
  return make_runtime_error(e.message( ));
}
}
