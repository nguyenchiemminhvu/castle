#include "sample_support.hpp"

#include "castle/sync/semaphore.hpp"

#include <stdint.h>

static uint32_t wait_calls = 0U;

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
        ++wait_calls;
    }
};

int main()
{
    castle::semaphore<2> slots(2U);

    CASTLE_SAMPLE_CHECK(slots.max() == 2U);
    CASTLE_SAMPLE_CHECK(slots.count() == 2U);

    slots.acquire();
    CASTLE_SAMPLE_CHECK(slots.count() == 1U);

    CASTLE_SAMPLE_CHECK(slots.try_acquire());
    CASTLE_SAMPLE_CHECK(slots.count() == 0U);
    CASTLE_SAMPLE_CHECK(!slots.try_acquire());

    CASTLE_SAMPLE_CHECK(slots.release());
    CASTLE_SAMPLE_CHECK(slots.release());
    CASTLE_SAMPLE_CHECK(!slots.release());

    castle::binary_semaphore signal(0U);
    CASTLE_SAMPLE_CHECK(!signal.try_acquire());
    CASTLE_SAMPLE_CHECK(signal.release());
    signal.acquire();
    CASTLE_SAMPLE_CHECK(signal.count() == 0U);

    castle::semaphore<1, counting_wait_policy> custom_slot(1U);
    custom_slot.acquire();
    CASTLE_SAMPLE_CHECK(wait_calls == 0U);
    CASTLE_SAMPLE_CHECK(custom_slot.release());

    return 0;
}
