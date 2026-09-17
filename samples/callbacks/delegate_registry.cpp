#include "sample_support.hpp"

#include "castle/callbacks/delegate.hpp"
#include "castle/callbacks/delegate_registry.hpp"

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
        total += (value * 2U);
    }
};
} // namespace

int main()
{
    using registry_type = castle::callbacks::delegate_registry<2U, void(uint32_t)>;

    registry_type registry;
    registry_type::error error = castle::status::unknown_error;

    CASTLE_SAMPLE_CHECK(registry.empty());
    CASTLE_SAMPLE_CHECK(registry.size() == 0U);
    CASTLE_SAMPLE_CHECK(registry.capacity() == 2U);

    castle::callbacks::subscription null_sub = registry.subscribe(nullptr, &error);
    CASTLE_SAMPLE_CHECK(!null_sub.valid());
    CASTLE_SAMPLE_CHECK(error == castle::status::invalid_callback);

    castle::callbacks::delegate_ptr<void(uint32_t)> free_delegate{&add_free};
    sink receiver{};
    castle::callbacks::delegate_member<sink, void(uint32_t)> member_delegate{receiver, &sink::add};

    castle::callbacks::subscription first = registry.subscribe(&free_delegate, &error);
    castle::callbacks::subscription first_copy = first;
    castle::callbacks::subscription second = registry.subscribe(&member_delegate, &error);

    CASTLE_SAMPLE_CHECK(first.valid());
    CASTLE_SAMPLE_CHECK(second.valid());
    CASTLE_SAMPLE_CHECK(error == castle::status::ok);
    CASTLE_SAMPLE_CHECK(registry.size() == 2U);

    registry.invoke(3U);
    CASTLE_SAMPLE_CHECK(g_sum == 3U);
    CASTLE_SAMPLE_CHECK(receiver.total == 6U);

    registry(1U);
    CASTLE_SAMPLE_CHECK(g_sum == 4U);
    CASTLE_SAMPLE_CHECK(receiver.total == 8U);

    castle::callbacks::subscription full = registry.subscribe(&free_delegate, &error);
    CASTLE_SAMPLE_CHECK(!full.valid());
    CASTLE_SAMPLE_CHECK(error == castle::status::full);

    CASTLE_SAMPLE_CHECK(first.unsubscribe() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(first_copy.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(registry.size() == 1U);

    castle::callbacks::subscription reused = registry.subscribe(&free_delegate, &error);
    CASTLE_SAMPLE_CHECK(reused.valid());
    CASTLE_SAMPLE_CHECK(reused.index() == 0U);
    CASTLE_SAMPLE_CHECK(reused.generation() != first.generation());

    registry.clear();
    CASTLE_SAMPLE_CHECK(registry.empty());
    CASTLE_SAMPLE_CHECK(second.unsubscribe() == castle::status::invalid_subscription);
    CASTLE_SAMPLE_CHECK(reused.unsubscribe() == castle::status::invalid_subscription);

    return 0;
}
