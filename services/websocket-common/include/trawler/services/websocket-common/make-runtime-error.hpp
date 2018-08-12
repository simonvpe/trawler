#pragma once
#include <boost/system/system_error.hpp>
#include <stdexcept>
#include <string>

namespace trawler {

std::exception_ptr
make_runtime_error(const std::string& message);

std::exception_ptr
make_runtime_error(const boost::system::error_code& ec);
  
}
