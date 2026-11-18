#include "sample_support.hpp"

#include "castle/callbacks/function_registry.hpp"

#include <stdint.h>

namespace
{
uint32_t g_sum = 0U;

void add_free(uint32_t value) noexcept
{
    g_sum += value;
}

struct sink
{
    uint32_t total = 0U;

    void add(uint32_t value) noexcept
    {
        total += (value * 3U);
    }
};
} // namespace

int main()
{
    using registry_type = castle::callbacks::function_registry<2U, void(uint32_t), 32U, 8U>;

    registry_type registry;
    registry_type::error error = castle::status::unknown_error;

    CASTLE_SAMPLE_CHECK(registry.capacity() == 2U);
    CASTLE_SAMPLE_CHECK(registry.empty());

    castle::callbacks::function<void(uint32_t), 32U, 8U> empty_callback{};
    castle::callbacks::subscription invalid = registry.subscribe(castle::move(empty_callback), &error);
    CASTLE_SAMPLE_CHECK(!invalid.valid());
    CASTLE_SAMPLE_CHECK(error == castle::status::invalid_callback);

    castle::callbacks::function<void(uint32_t), 32U, 8U> free_callback{&add_free};
    castle::callbacks::subscription first = registry.subscribe(castle::move(free_callback), &error);
    CASTLE_SAMPLE_CHECK(first.valid());
    CASTLE_SAMPLE_CHECK(error == castle::status::ok);

    sink receiver{};
    castle::callbacks::subscription second = registry.subscribe(
        [&receiver](uint32_t value) noexcept
        {
            receiver.add(value);
        },
        &error);
    CASTLE_SAMPLE_CHECK(second.valid());
    CASTLE_SAMPLE_CHECK(registry.size() == 2U);

    registry.invoke(2U);
    CASTLE_SAMPLE_CHECK(g_sum == 2U);
    CASTLE_SAMPLE_CHECK(receiver.total == 6U);

    registry(1U);
    CASTLE_SAMPLE_CHECK(g_sum == 3U);
    CASTLE_SAMPLE_CHECK(receiver.total == 9U);

    castle::callbacks::subscription full = registry.subscribe([](uint32_t) noexcept {}, &error);
    CASTLE_SAMPLE_CHECK(!full.valid());
    CASTLE_SAMPLE_CHECK(error == castle::status::full);

    CASTLE_SAMPLE_CHECK(first.unsubscribe() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(registry.size() == 1U);

    castle::callbacks::subscription reused = registry.subscribe(
        [](uint32_t value) noexcept
        {
            g_sum += (value * 10U);
        },
        &error);
    CASTLE_SAMPLE_CHECK(reused.valid());
    CASTLE_SAMPLE_CHECK(reused.index() == 0U);

    registry.clear();
    CASTLE_SAMPLE_CHECK(registry.empty());
    CASTLE_SAMPLE_CHECK(second.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(reused.unsubscribe() == castle::status::invalid_subscription);

    return 0;
}
