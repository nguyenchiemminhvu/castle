/**
 * Castle 2.0 sample: <castle/events/event_dispatcher.h>
 *
 * Scenario: route a sensor-ready interrupt event to task-level consumers using
 * compile-time event tags and fixed callback capacity.
 */
#include "sample_support.h"
#include "castle/events/event_config.h"
#include "castle/events/event_dispatcher.h"
#include "castle/callbacks/function.h"

#include <stdint.h>

struct SensorReadyEvent;
using sensor_ready_config = castle::events::event_config<
    SensorReadyEvent,
    4U,
    void(uint16_t)>;

namespace
{
uint16_t g_observed = 0U;

void on_sensor_ready(uint16_t sample_count) noexcept
{
    g_observed = sample_count;
}
} // namespace

int main()
{
    using dispatcher_type = castle::events::event_dispatcher<sensor_ready_config>;
    dispatcher_type dispatcher;
    castle::callbacks::function<void(uint16_t)> callback{on_sensor_ready};

    auto subscription = dispatcher.register_callback<SensorReadyEvent>(&callback);
    CASTLE_SAMPLE_CHECK(subscription.valid());

    CASTLE_SAMPLE_CHECK(dispatcher.dispatch_event<SensorReadyEvent>(17U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(g_observed == 17U);

    CASTLE_SAMPLE_CHECK(subscription.unsubscribe() == castle::status::ok);
    return 0;
}
