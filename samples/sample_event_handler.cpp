#include "castle/events/event_handler.hpp"
#include "castle/core/config.hpp"

#include <assert.h>
#include <stdint.h>
#include <unistd.h>

struct counter
{
    volatile uint32_t value = 0U;

    void add(uint32_t amount)
    {
        __sync_fetch_and_add(&value, amount);
    }
};

int main()
{
    castle::events::event_handler<8> handler; // maximum 8 pending events at any given time.
    assert(handler.initialized());
    assert(handler.running());
    assert(handler.capacity() == 8U);
    assert(handler.callback_storage_size() == castle::inplace_storage_reserved);

    counter c;

    assert(handler.post_event([&c] { c.add(1U); }) == castle::status::ok);
    assert(handler.post_event(&counter::add, &c, 2U) == castle::status::ok);
    assert(handler.post_delayed_event(30U, &counter::add, &c, 4U) == castle::status::ok);
    assert(handler.post_repeated_event(3U, 20U, &counter::add, &c, 8U) == castle::status::ok);

    ::usleep(180000U);

    // 1 + 2 + 4 + (8 * 3) = 31.
    assert(c.value == 31U);
    assert(handler.pending() == 0U);
    assert(handler.available() == 8U);

    assert(handler.shutdown() == castle::status::ok);
    assert(!handler.initialized());

    return 0;
}
