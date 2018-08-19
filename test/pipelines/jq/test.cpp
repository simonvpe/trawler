#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <trawler/pipelines/jq/jq.hpp>

static auto i = 0;

SCENARIO("dummy jq")
{
  GIVEN("a service packet and a jq pipeline")
  {
    auto jq = trawler::create_jq_pipeline(".key", "jq_" + std::to_string(i++));

    WHEN("passed several json objects in one string")
    {
      std::vector<std::string> results;

      rxcpp::observable<>::just(R"/({"key": "value1"} {"key": "value2"})/")
        .map([](auto s) {
          return trawler::ServicePacket{ trawler::ServicePacket::EStatus::DATA_TRANSMISSION, s };
        })
        .flat_map(jq)
        .subscribe([&](auto s) { results.push_back(s.get_payload( )); });

      THEN("each object should be emitted separately")
      {
        CHECK(results.size( ) == 2);
        CHECK(results[0] == R"/("value1")/");
        CHECK(results[1] == R"/("value2")/");
      }
    }

    WHEN("the result is null")
    {
      std::vector<std::string> results;

      rxcpp::observable<>::just(R"/({"other_key": "value1"} {"other_key": "value2"})/")
        .map([](auto s) {
          return trawler::ServicePacket{ trawler::ServicePacket::EStatus::DATA_TRANSMISSION, s };
        })
        .flat_map(jq)
        .subscribe([&](auto s) { results.push_back(s.get_payload( )); });

      THEN("nothing should be emitted") { CHECK(results.size( ) == 0); }
    }
  }
}
