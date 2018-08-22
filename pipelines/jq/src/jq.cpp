#include <memory>
#include <nlohmann/json.hpp>
#include <trawler/pipelines/jq/jq.hpp>

extern "C"
{
#include <jq.h>
}

namespace trawler {

std::shared_ptr<jq_state>
make_jq_state( )
{
  auto deleter = [](auto* p) { jq_teardown(&p); };
  return std::shared_ptr<jq_state>(jq_init( ), deleter);
}

std::shared_ptr<jv_parser>
make_jv_parser(int flags)
{
  auto deleter = [](auto* p) { jv_parser_free(p); };
  return std::shared_ptr<jv_parser>(jv_parser_new(flags), deleter);
}

struct jv_guard
{
  jv value;
  jv_guard(jv value)
    : value{ std::move(value) }
  {}
  ~jv_guard( ) { jv_free(value); }
  operator jv( ) { return value; }
};

std::function<rxcpp::observable<ServicePacket>(const ServicePacket&)>
create_jq_pipeline(const std::string& script, const Logger& logger)
{
  auto jq = trawler::make_jq_state( );
  auto args = jv_array( );
  jq_set_attr(jq.get( ), jv_string("PROGRAM_ORIGIN"), jv_string("myprog"));

  const auto compiled = jq_compile_args(jq.get( ), script.c_str( ), args);
  if (!compiled) {
    throw std::runtime_error{ "Failed to compile jq script" };
  }

  return [=](const ServicePacket& input) {
    return rxcpp::observable<>::create<ServicePacket>([=](auto subscriber) {
      auto parser = make_jv_parser(0);
      const auto payload = input.get_payload_as<std::string>( );
      jv_parser_set_buf(parser.get( ), payload.c_str( ), payload.size( ), 0);

      while (true) {
        auto value = jv_parser_next(parser.get( ));

        if (!jv_is_valid(value)) {
          break;
        }

        jq_start(jq.get( ), value, 0);
        while (true) {
          auto result = jq_next(jq.get( ));

          if (!jv_is_valid(result)) {
            break;
          }

          auto dump = jv_guard{ jv_dump_string(result, 0) };
          auto result_string = std::string{ jv_string_value(dump) };

          if (result_string != "null") {
            logger.debug("Emitting " + result_string);
            auto json = nlohmann::json::parse(result_string);
            subscriber.on_next(input.with_payload({ json }));
          }
        }
      }
      subscriber.on_completed( );
    });
  };
}
}
