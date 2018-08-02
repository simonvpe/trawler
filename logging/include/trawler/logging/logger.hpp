#pragma once
#include <memory>

namespace trawler {
class Logger
{
  std::shared_ptr<class LoggerBackend> backend;

public:
  enum class ELogLevel
  {
    DEBUG,
    INFO,
    CRITICAL
  };

  Logger(const std::string& label);
  Logger( ) = default;
  Logger(Logger&&) = default;
  Logger(const Logger&) = default;
  Logger& operator=(Logger&&) = default;
  Logger& operator=(const Logger&) = default;
  ~Logger( ) = default;

  void debug(const std::string&) const;
  void info(const std::string&) const;
  void critical(const std::string&) const;

  static void set_log_level(ELogLevel);
};
}
