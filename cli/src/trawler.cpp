#include <fstream>
#include <iostream>
#include <string>
#include <trawler/cli/parse-configuration.hpp>
#include <trawler/cli/parse-options.hpp>
#include <trawler/logging/logger.hpp>
#include <vector>

using namespace trawler;

auto
print_usage(std::ostream& stream, const boost::program_options::options_description& description)
{
  stream << "Usage: trawler [OPTIONS] configuration-file\n";
  stream << description << '\n';
}

auto
print_exception(const std::exception_ptr& err)
{
  try {
    std::rethrow_exception(err);
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what( ) << "\n\n";
  }
}

std::tuple<std::string, std::exception_ptr>
get_config_file(const boost::program_options::variables_map& vm)
{
  if (vm.count("config") < 1) {
    return { {}, std::make_exception_ptr(std::runtime_error{ "no configuration-file specified" }) };
  }
  return { vm["config"].as<std::vector<std::string>>( ).front( ), nullptr };
}

std::exception_ptr
configure_loglevel(const boost::program_options::variables_map& vm)
{
  const auto loglevel = vm["loglevel"].as<std::string>( );

  if (loglevel == "info") {
    Logger::set_log_level(Logger::ELogLevel::INFO);
    return nullptr;
  }

  if (loglevel == "debug") {
    Logger::set_log_level(Logger::ELogLevel::DEBUG);
    return nullptr;
  }

  if (loglevel == "critical") {
    Logger::set_log_level(Logger::ELogLevel::CRITICAL);
    return nullptr;
  }

  return std::make_exception_ptr(std::runtime_error{ "unknown loglevel " + loglevel });
}

int
main(int argc, const char* argv[])
{
  Logger logger{ "trawler" };

  const auto [vm, description, err] = parse_options(argc, argv);

  if (vm.count("help")) {
    print_usage(std::cout, description);
    return 0;
  }

  if (err) {
    print_usage(std::cerr, description);
    print_exception(err);
    return 1;
  }

  {
    const auto err = configure_loglevel(vm);
    if (err) {
      print_usage(std::cerr, description);
      print_exception(err);
      return 2;
    }
  }

  std::string configuration;
  {
    auto [filename, err] = get_config_file(vm);
    if (err) {
      print_usage(std::cerr, description);
      print_exception(err);
      return 3;
    }

    try {
      std::fstream file{ filename, std::ios::in };
      if (!file) {
        throw std::runtime_error("failed to open file " + filename);
      }
      configuration = std::string{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>( ) };
    } catch (...) {
      print_usage(std::cerr, description);
      print_exception(std::current_exception( ));
      return 4;
    }
  }

  parse_configuration(configuration);

  return 0;
}
