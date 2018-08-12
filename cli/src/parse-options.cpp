#include <string>
#include <trawler/cli/parse-options.hpp>
#include <vector>

namespace trawler {

std::tuple<variables_map, options_description, std::exception_ptr>
parse_options(int argc, const char* argv[])
{
  namespace po = boost::program_options;

  const auto max_nof_files = 1;
  po::positional_options_description positional;
  positional.add("config", max_nof_files);

  po::options_description desc{ "Options" };
  desc.add_options( )("help,h", "print usage")(
    "loglevel", po::value<std::string>()->default_value("info"), "debug|info|critical");
  
  po::options_description hidden{ "Hidden options" };
  hidden.add_options( )("config", po::value<std::vector<std::string>>( ), "configuration file");

  po::options_description options;
  options.add(desc).add(hidden);

  po::variables_map vm;
  try {
    po::store(po::command_line_parser(argc, argv).options(options).positional(positional).run( ), vm);
    po::notify(vm);
  } catch (...) {
    return { vm, desc, std::current_exception( ) };
  }

  return { vm, desc, nullptr };
}
}
