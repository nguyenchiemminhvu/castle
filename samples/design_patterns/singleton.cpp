#include "sample_support.hpp"

#include "castle/design_patterns/singleton.hpp"

#include <stdint.h>

struct telemetry_hub
{
    explicit telemetry_hub(uint32_t initial) : sequence(initial) {}
    ~telemetry_hub() noexcept = default;

    uint32_t sequence;
};

using telemetry_hub_singleton = castle::design_patterns::singleton<telemetry_hub>;

int main()
{
    CASTLE_SAMPLE_CHECK(!telemetry_hub_singleton::is_valid());
    telemetry_hub_singleton::create(1U);
    telemetry_hub_singleton::instance().sequence += 1U;
    CASTLE_SAMPLE_CHECK(telemetry_hub_singleton::instance().sequence == 2U);
    telemetry_hub_singleton::destroy();
    CASTLE_SAMPLE_CHECK(!telemetry_hub_singleton::is_valid());
    return 0;
}
