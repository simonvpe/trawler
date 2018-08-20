#pragma once
#include <boost/system/system_error.hpp>
#include <stdexcept>
#include <string>

namespace trawler {

inline std::exception_ptr
make_runtime_error(const std::string& message)
{
  return std::make_exception_ptr(std::runtime_error{ message });
}

inline std::exception_ptr
make_runtime_error(const boost::system::error_code& ec)
{
  return make_runtime_error(ec.message( ));
}
}
