/**
 * Castle 2.0 sample: <castle/callbacks/callback_registry.h>
 *
 * Scenario: a fixed-capacity telemetry fan-out. The registry stores only
 * non-owning callback pointers, so application lifetime remains explicit.
 */
#include "sample_support.h"
#include "castle/callbacks/callback_registry.h"
#include "castle/callbacks/function.h"

#include <stdint.h>

namespace
{
uint32_t g_delivered = 0U;

void on_telemetry(uint32_t value) noexcept
{
    g_delivered += value;
}
} // namespace

int main()
{
    using callback_type = castle::callbacks::function<void(uint32_t)>;
    using registry_type = castle::callbacks::callback_registry<4U, void(uint32_t)>;

    callback_type callback{on_telemetry};
    registry_type registry;
    registry_type::error error = castle::status::ok;

    castle::callbacks::callback_subscription subscription = registry.subscribe(&callback, &error);

    CASTLE_SAMPLE_CHECK(error == castle::status::ok);
    CASTLE_SAMPLE_CHECK(subscription.valid());
    CASTLE_SAMPLE_CHECK(registry.size() == 1U);

    registry.invoke(3U);
    CASTLE_SAMPLE_CHECK(g_delivered == 3U);

    CASTLE_SAMPLE_CHECK(subscription.unsubscribe() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(registry.empty());
    return 0;
}
