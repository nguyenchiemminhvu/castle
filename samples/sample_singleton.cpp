#include "sample_support.h"

#include "castle/design_patterns/singleton.h"

// Scenario: providing one explicitly controlled device-service instance without heap allocation.
#include <stdint.h>

struct TelemetryHub
{
    explicit TelemetryHub(uint32_t initial) : sequence(initial) {}
    ~TelemetryHub() noexcept = default;
    uint32_t sequence;
};

using telemetry_hub = castle::design_patterns::singleton<TelemetryHub>;

int main()
{
    CASTLE_SAMPLE_CHECK(!telemetry_hub::is_valid());
    telemetry_hub::create(1U);
    telemetry_hub::instance().sequence += 1U;
    CASTLE_SAMPLE_CHECK(telemetry_hub::instance().sequence == 2U);
    telemetry_hub::destroy();
    CASTLE_SAMPLE_CHECK(!telemetry_hub::is_valid());
    return 0;
}
