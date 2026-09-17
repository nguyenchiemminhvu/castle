/**
 * Castle 2.0 sample: <castle/events/inplace_event_dispatcher.h>
 *
 * Scenario: a stateful callback is kept in deterministic inline storage while
 * the event key and payload remain statically described by event_config.
 */
#include "sample_support.h"
#include "castle/events/event_config.h"
#include "castle/events/inplace_event_dispatcher.h"

#include <stdint.h>

struct SensorReadyEvent;

using sensor_ready_config =
    castle::events::event_config<
        SensorReadyEvent,
        4U,
        void(uint16_t),
        32U,
        alignof(uint32_t)>;

int main()
{
    castle::events::inplace_event_dispatcher<sensor_ready_config> dispatcher;
    uint16_t observed = 0U;

    auto subscription = dispatcher.register_callback<SensorReadyEvent>(
        [&observed](uint16_t sample_count) noexcept
        {
            observed = sample_count;
        }
    );

    CASTLE_SAMPLE_CHECK(subscription.valid());
    CASTLE_SAMPLE_CHECK(dispatcher.dispatch_event<SensorReadyEvent>(23U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(observed == 23U);
    CASTLE_SAMPLE_CHECK(subscription.unsubscribe() == castle::status::ok);
    return 0;
}
