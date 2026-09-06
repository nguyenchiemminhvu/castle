#include "sample_support.h"

#include "castle/design_patterns/observer.h"

// Scenario: notifying a fixed set of observers when a sensor value changes.
#include <stdint.h>

struct TemperatureObserver : castle::design_patterns::observer<uint16_t>
{
    void notify(uint16_t const& value) override { latest = value; }
    uint16_t latest = 0U;
};

int main()
{
    TemperatureObserver observer;
    castle::design_patterns::observable<TemperatureObserver, 2U> sensor_bus;
    CASTLE_SAMPLE_CHECK(sensor_bus.add_observer(&observer));
    sensor_bus.notify_observers<uint16_t>(42U);
    CASTLE_SAMPLE_CHECK(observer.latest == 42U);
    CASTLE_SAMPLE_CHECK(sensor_bus.remove_observer(&observer));
    return 0;
}
