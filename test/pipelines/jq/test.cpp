#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest.h>
#include <trawler/pipelines/jq/jq.hpp>

SCENARIO("dummy jq")
{
  GIVEN("a service packet and a jq pipeline")
  {
    // auto jq = create_jq_obs(".key");

    // rxcpp::observable<>::just(R"/({"key": "value1"} {"key": "value2"})/").flat_map(jq).subscribe([](auto s) {
    //   std::cout << "RESULT: " << s << '\n';
    // });
    /*
    Logger::set_log_level(Logger::ELogLevel::DEBUG);

    const auto input = ServicePacket(ServicePacket::EStatus::DATA_TRANSMISSION, R"/({"key": "value"})/");
    const auto jq_pipeline = create_jq_pipeline(".key");

    jq_pipeline(input).take(1).subscribe([](auto sp) {
                                   std::cout << sp.get_payload() << '\n';
                                 });
    CHECK(false);
    */
  }
}
