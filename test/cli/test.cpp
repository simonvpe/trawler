#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <trawler/cli/parse-configuration.hpp>
#include <trawler/cli/parse-options.hpp>
#include <vector>

auto
parse(std::vector<const char*> args)
{
  return trawler::parse_options(args.size( ), args.data( ));
}

SCENARIO("Options")
{

  WHEN("--help")
  {
    const auto [vm, desc, err] = parse({ "progname", "--help" });

    THEN("the help key should be found")
    {
      CHECK(vm.count("help"));

      AND_THEN("there should be no error") { CHECK(err == nullptr); }
    }
  }

  GIVEN("a configuration file as positional argument")
  {
    const auto [vm, desc, err] = parse({ "progname", "config.yaml" });

    THEN("the file name should be found in the variables map")
    {
      REQUIRE(vm.count("config"));
      const auto files = vm["config"].as<std::vector<std::string>>( );
      CHECK(files.size( ) == 1);
      CHECK(files[0] == "config.yaml");

      AND_THEN("there should be no error") { CHECK(err == nullptr); }
    }
  }

  GIVEN("several configuration files as input")
  {
    const auto [vm, desc, err] = parse({ "progname", "config1.yaml", "config2.yaml" });
    THEN("there should be an error") { CHECK(err != nullptr); }
  }
}

SCENARIO("configuration")
{
  GIVEN("a websocket client service")
  {
    const auto configuration = trawler::parse_configuration(R"#(
    services:
      - name: my-websocket-client
        service: websocket-client
        host: myhost.com
        port: 111
        target: /target
        ssl: false
    )#");
    const auto services = configuration.services;
    REQUIRE(services.size( ) == 1);

    const auto service = services.front( );
    REQUIRE(std::holds_alternative<trawler::config::websocket_client_service_t>(service));

    const auto websocket_client_service = std::get<trawler::config::websocket_client_service_t>(service);
    CHECK(websocket_client_service.name == "my-websocket-client");
    CHECK(websocket_client_service.service == "websocket-client");
    CHECK(websocket_client_service.host == "myhost.com");
    CHECK(websocket_client_service.port == 111);
    CHECK(websocket_client_service.target == "/target");
    CHECK(websocket_client_service.ssl == false);
  }

  GIVEN("a pipeline")
  {
    const auto configuration = trawler::parse_configuration(R"#(
    pipelines:
      - name: my-pipeline
        pipeline: endpoint
        source: some-source
        event: some-event
        data: some-data
    )#");
    const auto pipelines = configuration.pipelines;
    REQUIRE(pipelines.size( ) == 1);

    const auto pipeline = pipelines.front( );
    REQUIRE(std::holds_alternative<trawler::config::endpoint_pipeline_t>(pipeline));

    const auto endpoint_pipeline = std::get<trawler::config::endpoint_pipeline_t>(pipeline);
    CHECK(endpoint_pipeline.name == "my-pipeline");
    CHECK(endpoint_pipeline.pipeline == "endpoint");
    CHECK(endpoint_pipeline.source == "some-source");
    CHECK(endpoint_pipeline.event == "some-event");
    CHECK(endpoint_pipeline.data == "some-data");
  }
}
