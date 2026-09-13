#include "sample_support.hpp"

#include "castle/utility/forward.hpp"

// Scenario: preserving value category in a generic embedded factory.
#include <stdint.h>

template <typename T>
void accept(T&& value) noexcept
{
    CASTLE_SAMPLE_CHECK(value == 7U);
}

int main()
{
    uint32_t value = 7U;
    accept(castle::forward<uint32_t&>(value));
    return 0;
}
