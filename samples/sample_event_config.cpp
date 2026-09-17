/**
 * Castle 2.0 sample: <castle/events/event_config.h>
 *
 * Scenario: declare an event contract at compile time. The dispatcher samples
 * consume these same configuration types, keeping capacities deterministic.
 */
#include "sample_support.hpp"
#include "castle/events/event_config.hpp"

#include <stdint.h>

struct SensorReadyEvent;

using sensor_ready_config =
    castle::events::event_config<
        SensorReadyEvent,
        4U,
        void(uint16_t)>;

int main()
{
    // Instantiating the configuration is itself the compile-time contract.
    sensor_ready_config* config = nullptr;
    (void)config;
    return 0;
}
