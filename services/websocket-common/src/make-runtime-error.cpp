#include <trawler/services/websocket-common/make-runtime-error.hpp>

namespace trawler {
std::exception_ptr
make_runtime_error(const std::string& message)
{
  return std::make_exception_ptr(std::runtime_error{ message });
}

std::exception_ptr
make_runtime_error(const boost::system::error_code& ec)
{
  return make_runtime_error(ec.message( ));
}
}
