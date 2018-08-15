#include <spdlog/details/registry.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <trawler/logging/logger.hpp>

namespace trawler {

class LoggerBackend
  : public spdlog::logger
  , public std::enable_shared_from_this<LoggerBackend>
{
  using sink_t = spdlog::sinks::stdout_color_sink_mt;

public:
  explicit LoggerBackend(std::string name)
    : spdlog::logger{ std::move(name), std::make_shared<sink_t>( ) }
  {}

  auto register_and_init( )
  {
    std::shared_ptr<spdlog::logger> me = shared_from_this( );
    spdlog::details::registry::instance( ).register_and_init(me);
  }
};

Logger::Logger(const std::string& label)
  : backend{ std::make_shared<LoggerBackend>(label) }
  , label{ label }
{
  backend->register_and_init( );
}

void
Logger::critical(const std::string& s) const
{
  if (backend) {
    backend->critical(s);
  }
}

void
Logger::info(const std::string& s) const
{
  if (backend) {
    backend->info(s);
  }
}

void
Logger::debug(const std::string& s) const
{
  if (backend) {
    backend->debug(s);
  }
}

const std::string&
Logger::get_label( ) const
{
  return label;
}

void
Logger::set_log_level(ELogLevel level)
{
  switch (level) {
    case ELogLevel::CRITICAL:
      spdlog::set_level(spdlog::level::critical);
      break;
    case ELogLevel::INFO:
      spdlog::set_level(spdlog::level::info);
      break;
    case ELogLevel::DEBUG:
      spdlog::set_level(spdlog::level::debug);
      break;
    default:
      throw std::runtime_error("Unknown log level");
  }
}
}
