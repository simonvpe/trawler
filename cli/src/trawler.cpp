#include <fstream>
#include <iostream>
#include <string>
#include <trawler/cli/parse-configuration.hpp>
#include <trawler/cli/parse-options.hpp>
#include <trawler/cli/spawn-endpoints.hpp>
#include <trawler/cli/spawn-pipelines.hpp>
#include <trawler/cli/spawn-services.hpp>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-context.hpp>
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
    return { "", std::make_exception_ptr(std::runtime_error{ "no configuration-file specified" }) };
  }
  return { vm["config"].as<std::vector<std::string>>( ).front( ), nullptr };
}

std::exception_ptr
configure_loglevel(const std::string& loglevel)
{
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

template<typename TimeDelta>
void
wait_for_unsubscribe(std::vector<rxcpp::subscription>& subscriptions, TimeDelta timeout)
{
  using namespace std::chrono_literals;
  auto is_subscribed = [](const auto& sub) { return sub.is_subscribed( ); };
  auto unsubscribe = [](auto& sub) { sub.unsubscribe( ); };

  const auto stop_time = std::chrono::system_clock::now( ) + timeout;

  while (std::all_of(cbegin(subscriptions), cend(subscriptions), is_subscribed)) {
    if (timeout > 0ns && std::chrono::system_clock::now( ) > stop_time) {
      std::for_each(begin(subscriptions), end(subscriptions), unsubscribe);
    }
    std::this_thread::yield( );
    std::this_thread::sleep_for(100ms);
  }
}

void
wait_for_unsubscribe(std::vector<rxcpp::subscription>& subscriptions)
{
  using namespace std::chrono_literals;
  wait_for_unsubscribe(subscriptions, 0ms);
}

int
main(int argc, const char* argv[]) // NOLINT(readability-function-size)
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
    const auto err = configure_loglevel(vm["loglevel"].as<std::string>( ));
    if (err) {
      print_usage(std::cerr, description);
      print_exception(err);
      return 2;
    }
  }

  std::string configuration_string;
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
      configuration_string = std::string{ std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>( ) };
    } catch (...) {
      print_usage(std::cerr, description);
      print_exception(std::current_exception( ));
      return 4;
    }
  }

  const auto configuration = parse_configuration(configuration_string);

  auto context = make_service_context( );

  auto services = spawn_services(context, configuration.services, logger);

  auto pipelines = spawn_pipelines(services, configuration.pipelines, logger);

  auto subscriptions = spawn_endpoints(services, pipelines, configuration.endpoints, logger);

  using namespace std::chrono_literals;
  wait_for_unsubscribe(subscriptions, 2s);

  return 0;
}
