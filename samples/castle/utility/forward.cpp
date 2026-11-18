#include "sample_support.hpp"

#include "castle/utility/forward.hpp"

#include <stdint.h>

static uint32_t category(uint32_t&) noexcept
{
    return 1U;
}

static uint32_t category(uint32_t&&) noexcept
{
    return 2U;
}

template <typename T>
static uint32_t relay(T&& value) noexcept
{
    return category(castle::forward<T>(value));
}

int main()
{
    uint32_t value = 7U;
    CASTLE_SAMPLE_CHECK(&castle::forward<uint32_t&>(value) == &value);
    CASTLE_SAMPLE_CHECK(relay(value) == 1U);
    CASTLE_SAMPLE_CHECK(relay(static_cast<uint32_t&&>(value)) == 2U);
    return 0;
}
