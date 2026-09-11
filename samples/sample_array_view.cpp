#include "sample_support.h"

#include "castle/container/array_view.h"

// Scenario: passing a non-owning view of a DMA/ADC buffer to a processing routine.
int main()
{
    uint16_t samples[6U] = {1U, 2U, 3U, 4U, 5U, 6U};
    castle::container::array_view<uint16_t> view(samples, 6U);
    const auto tail = view.subview(2U, 3U);
    CASTLE_SAMPLE_CHECK(tail.size() == 3U);
    tail[0] = 30U;
    CASTLE_SAMPLE_CHECK(samples[2] == 30U);
    return 0;
}
