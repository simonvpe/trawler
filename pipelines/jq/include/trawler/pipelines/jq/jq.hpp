#pragma once
#include <functional>
#include <memory>
#include <rxcpp/rx.hpp>
#include <string>
#include <trawler/logging/logger.hpp>
#include <trawler/services/service-packet.hpp>

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

auto
create_jq_pipeline(const std::string& script, const Logger& logger = { "jq" })
{
  return [=](const ServicePacket& input) {
    auto jq = trawler::make_jq_state( );
    auto args = jv_array( );
    jq_set_attr(jq.get( ), jv_string("PROGRAM_ORIGIN"), jv_string("myprog"));

    return rxcpp::observable<>::create<ServicePacket>([=](auto subscriber) {
      const auto compiled = jq_compile_args(jq.get( ), script.c_str( ), args);
      jv_free(args);

      if (!compiled) {
        logger.critical("Failed to compile jq script");
        subscriber.on_error(std::make_exception_ptr(std::runtime_error("Failed to compile jq script")));
        return;
      }

      auto parser = make_jv_parser(0);
      jv_parser_set_buf(parser.get( ), input.get_payload( ).c_str( ), input.get_payload( ).size( ), 0);

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

          auto dump = jv_dump_string(result, 0);
          subscriber.on_next(input.with_payload(std::string{ jv_string_value(dump) }));

          jv_free(dump);
          jv_free(result);
        }
        jv_free(value);
      }
      subscriber.on_completed( );
    });
  };
}
}
