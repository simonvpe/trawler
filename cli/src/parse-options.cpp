#include <trawler/cli/parse-options.hpp>
#include <vector>
#include <string>

namespace trawler {
  
boost::program_options::variables_map parse_options(int argc, const char* argv[])
{
  namespace po = boost::program_options;

  const auto max_nof_files = 1;
  po::positional_options_description positional;
  positional.add("config", max_nof_files);
  
  po::options_description desc{"Options"};
  desc.add_options()
    ("help,h", "print usage")
    ("config", po::value< std::vector<std::string> >(), "configuration file")
    ;

  po::variables_map vm;
  po::store(po::command_line_parser(argc, argv).options(desc).positional(positional).run(), vm);
  po::notify(vm);
  
  return vm;
}

}
