#include "sample_support.hpp"

#include "castle/callbacks/callback_dispatcher.hpp"
#include "castle/callbacks/function.hpp"

#include <stdint.h>

namespace
{
using callback_type = castle::callbacks::function<void(uint32_t), 64U, 8U>;
using dispatcher_type = castle::callbacks::callback_dispatcher<callback_type>;

struct owner
{
    callback_type callback{};
    dispatcher_type dispatcher{};
    uint32_t total = 0U;
    bool saw_running = false;
};
} // namespace

int main()
{
    owner o{};
    CASTLE_SAMPLE_CHECK(!o.dispatcher.is_dispatching());

    o.dispatcher.replace(o.callback, callback_type{[&o](uint32_t value) noexcept { o.total += value; }});
    o.dispatcher.invoke(o.callback, 2U);
    CASTLE_SAMPLE_CHECK(o.total == 2U);

    // The callback swaps itself out while running; the new callable is installed after it returns.
    o.dispatcher.replace(o.callback, callback_type{[&o](uint32_t value) noexcept {
        o.saw_running = o.dispatcher.is_dispatching() && o.dispatcher.is_running(o.callback);
        o.total += value;
        o.dispatcher.replace(o.callback, callback_type{[&o](uint32_t v) noexcept { o.total += (v * 10U); }});
    }});
    o.dispatcher.invoke(o.callback, 3U);
    CASTLE_SAMPLE_CHECK(o.saw_running);
    CASTLE_SAMPLE_CHECK(o.total == 5U);
    CASTLE_SAMPLE_CHECK(!o.dispatcher.is_dispatching());

    o.dispatcher.invoke(o.callback, 4U);
    CASTLE_SAMPLE_CHECK(o.total == 45U);

    // A callback that clears itself leaves the slot empty once it returns.
    o.dispatcher.replace(o.callback, callback_type{[&o](uint32_t value) noexcept {
        o.total += value;
        o.dispatcher.clear(o.callback);
    }});
    o.dispatcher.invoke(o.callback, 1U);
    CASTLE_SAMPLE_CHECK(o.total == 46U);
    CASTLE_SAMPLE_CHECK(!static_cast<bool>(o.callback));

    return 0;
}
