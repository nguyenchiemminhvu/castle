#include "sample_support.hpp"

#include "castle/sync/semaphore.hpp"

#include <stdint.h>

// ---------------------------------------------------------------------------
// Scenario: gate access to a fixed pool of 4 interchangeable resource slots
// (e.g. DMA channels, communication buffers, worker tasks), similar to a
// Zephyr k_sem or POSIX sem_t used as a resource-counting semaphore.
// ---------------------------------------------------------------------------
static castle::semaphore<4> resource_slots(4U);

static void use_resource_slot()
{
    resource_slots.acquire();

    // Access one of the up to 4 interchangeable resource slots.

    CASTLE_SAMPLE_CHECK(resource_slots.release());
}

// ---------------------------------------------------------------------------
// Scenario: an ISR-to-task signal using a single-permit binary_semaphore.
// The ISR never calls acquire(); it only ever releases a permit to wake the
// waiting task, which distinguishes a semaphore from castle::mutex.
// ---------------------------------------------------------------------------
static castle::binary_semaphore data_ready(0U);

static void simulated_isr_signal()
{
    CASTLE_SAMPLE_CHECK(data_ready.release());
}

static void wait_for_data()
{
    data_ready.acquire();

    // Process the data made available by the interrupt.
}

// ---------------------------------------------------------------------------
// Scenario: swap the default CPU-relax spin for a custom WaitPolicy.
//
// castle::semaphore cannot itself implement real thread scheduling: doing so
// needs an actual kernel (task state, stacks, a wait queue) that a
// header-only, no-heap, no-virtual, freestanding template library does not
// own. Instead, WaitPolicy is a compile-time extension point: any type with
// a `static void wait() noexcept` can be plugged in, letting an RTOS
// integration yield to its real scheduler instead of spinning.
//
// This sample's policy just counts retries (no OS present here); a
// production integration would call e.g. `k_yield()` (Zephyr) or
// `taskYIELD()` (FreeRTOS) from wait() instead.
// ---------------------------------------------------------------------------
static uint32_t retry_count = 0U;

struct counting_wait_policy
{
    static void wait() CASTLE_NOEXCEPT
    {
        ++retry_count;
    }
};

static castle::semaphore<1, counting_wait_policy> single_slot(1U);

static void use_single_slot_with_custom_wait_policy()
{
    single_slot.acquire();

    // Uncontended fast path: WaitPolicy::wait() is never invoked here.
    CASTLE_SAMPLE_CHECK(retry_count == 0U);

    // Access the single resource slot.

    CASTLE_SAMPLE_CHECK(single_slot.release());
}

int main()
{
    CASTLE_SAMPLE_CHECK(resource_slots.count() == 4U);

    use_resource_slot();
    use_resource_slot();
    use_resource_slot();
    use_resource_slot();

    CASTLE_SAMPLE_CHECK(resource_slots.count() == 4U);

    simulated_isr_signal();
    wait_for_data();

    use_single_slot_with_custom_wait_policy();

    return 0;
}
