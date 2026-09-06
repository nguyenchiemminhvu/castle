#include "sample_support.h"

#include "castle/callbacks/callback_subscription.h"

// Scenario: holding a deterministic callback-registration token in an embedded component.
#include <stdint.h>

int main()
{
    // A subscription token is intentionally a value object. Concrete registries
    // own the callback and invalidate the token when the subscription is removed.
    castle::callbacks::callback_subscription subscription{};
    CASTLE_SAMPLE_CHECK(!subscription.valid());
    return 0;
}
