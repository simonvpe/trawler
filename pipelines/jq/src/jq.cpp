#include <memory>
#include <trawler/pipelines/jq/jq.hpp>

extern "C"
{
#include <jq.h>
}

namespace trawler {

auto
make_jq_state( )
{
  auto deleter = [](auto* p) { jq_teardown(&p); };
  return std::shared_ptr<jq_state>(jq_init( ), deleter);
}

auto
make_jv_parser(int flags)
{
  auto deleter = [](auto* p) { jv_parser_free(p); };
  return std::shared_ptr<jv_parser>(jv_parser_new(flags), deleter);
}

auto
compile_script(std::weak_ptr<jq_state> jqw, const std::string& script, const std::string& name)
{
  auto jq = jqw.lock( ).get( );

  jq_set_attr(jq, jv_string("PROGRAM_ORIGIN"), jv_string(name.c_str( )));

  const auto compiled = jq_compile_args(jq, script.c_str( ), jv_array( ));

  if (!compiled) {
    throw std::runtime_error("Failed to compile jq script in " + name);
  }
}

std::function<ServicePacket(ServicePacket)>
create_jq_pipeline(const std::string& script, const Logger& logger)
{
  auto jq = make_jq_state( );

  compile_script(jq, script, logger.get_label( ));

  return [jq, script, logger](const auto& x) {
    logger.debug("in: " + x.get_payload( ));

    auto parser = make_jv_parser(0);
    jv_parser_set_buf(parser.get( ), x.get_payload( ).c_str( ), x.get_payload( ).size( ), 0);

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
        logger.debug("out: " + std::string{ jv_string_value(jv_dump_string(result, 0)) });
      }
    }

    return x;
  };
}
}
