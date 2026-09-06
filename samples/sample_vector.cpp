#include "sample_support.h"

#include "castle/container/vector.h"

// Scenario: growing a deterministic in-object vector of samples up to a compile-time bound.
int main()
{
    castle::container::vector<uint16_t, 4U> values;
    CASTLE_SAMPLE_CHECK(values.emplace_back(10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.push_back(20U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(values.size() == 2U);
    values[1U] = 25U;
    CASTLE_SAMPLE_CHECK(values.back() == 25U);
    CASTLE_SAMPLE_CHECK(values.pop_back() == castle::status::ok);
    return 0;
}
