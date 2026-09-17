#include "sample_support.hpp"

#include "castle/callbacks/subscription.hpp"

// Scenario: holding a deterministic callback-subscription token in an embedded component.
#include <stdint.h>

int main()
{
    // A subscription token is intentionally a value object. Concrete registries
    // own the callback and invalidate the token when the subscription is removed.
    castle::callbacks::subscription subscription{};
    CASTLE_SAMPLE_CHECK(!subscription.valid());
    return 0;
}
