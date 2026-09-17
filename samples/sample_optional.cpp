#include "sample_support.hpp"

#include "castle/utility/optional.hpp"

// Scenario: representing an optional sensor reading without a sentinel value or allocation.
#include <stdint.h>
int main()
{
    castle::optional<uint32_t> value;
    CASTLE_SAMPLE_CHECK(!value.has_value());
    value.emplace(42U);
    CASTLE_SAMPLE_CHECK(value.has_value());
    CASTLE_SAMPLE_CHECK(value.value() == 42U);
    CASTLE_SAMPLE_CHECK(value.value_or(7U) == 42U);
    value.reset();
    CASTLE_SAMPLE_CHECK(!value);
    return 0;
}
