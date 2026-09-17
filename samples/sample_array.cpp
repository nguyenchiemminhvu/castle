#include "sample_support.h"

#include "castle/container/array.h"

// Scenario: storing a fixed set of ADC channels without any dynamic allocation.
int main()
{
    castle::container::array<uint32_t, 4U> adc{100U, 200U, 300U, 400U};
    CASTLE_SAMPLE_CHECK(adc.size() == 4U);
    CASTLE_SAMPLE_CHECK(adc.front() == 100U);
    CASTLE_SAMPLE_CHECK(adc.back() == 400U);
    adc[2] = 330U;
    uint32_t sum = 0U;
    for (auto value : adc) sum += value;
    CASTLE_SAMPLE_CHECK(sum == 1030U);
    return 0;
}
