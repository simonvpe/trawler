#pragma once

#include <boost/program_options.hpp>
#include <stdexcept>
#include <tuple>

namespace trawler {

using boost::program_options::options_description;
using boost::program_options::variables_map;

std::tuple<variables_map, options_description, std::exception_ptr>
parse_options(int argc, const char* argv[]);
}
