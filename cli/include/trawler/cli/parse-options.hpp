#pragma once

#include <boost/program_options.hpp>

namespace trawler {
  
boost::program_options::variables_map parse_options(int argc, const char* argv[]);

}
