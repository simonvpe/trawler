#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <trawler/cli/parse-options.hpp>
#include <vector>

auto parse(std::vector<const char*> args)
{
  return trawler::parse_options(args.size(), args.data());
}

SCENARIO("Options") {

  WHEN("--help") {
    const auto vm = parse({"progname", "--help"});
    THEN("the help key should be found") {
      CHECK(vm.count("help"));
    }
  }

  GIVEN("a configuration file as positional argument") {
    const auto vm = parse({"progname", "config.yaml"});
    THEN("the file name should be found in the variables map") {
      REQUIRE(vm.count("config"));
      const auto files = vm["config"].as<std::vector<std::string>>();
      CHECK(files.size() == 1);
      CHECK(files[0] == "config.yaml");
    }
  }

  GIVEN("several configuration files as input") {
    THEN("an exception should be thrown") {
      CHECK_THROWS(parse({"progname", "config1.yaml", "config2.yaml"}));
    }
  }
}
