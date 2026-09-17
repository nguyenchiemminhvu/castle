/**
 * Castle 2.0 sample: <castle/callbacks/function_registry.h>
 *
 * Scenario: a small ISR/task callback fan-out where stateful lambdas are stored
 * in caller-owned fixed inline buffers. No heap allocation is required.
 */
#include "sample_support.hpp"
#include "castle/callbacks/function_registry.hpp"

#include <stdint.h>

int main()
{
    using registry_type =
        castle::callbacks::function_registry<
            4U,
            void(uint32_t)>;

    registry_type registry;
    uint32_t delivered = 0U;

    auto subscription = registry.subscribe(
        [&delivered](uint32_t value) noexcept
        {
            delivered += value;
        }
    );

    CASTLE_SAMPLE_CHECK(subscription.valid());
    CASTLE_SAMPLE_CHECK(registry.size() == 1U);

    registry.invoke(9U);
    CASTLE_SAMPLE_CHECK(delivered == 9U);

    CASTLE_SAMPLE_CHECK(subscription.unsubscribe() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(registry.empty());
    return 0;
}
