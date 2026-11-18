#include "sample_support.hpp"

#include "castle_ext/statistic/sample_window.hpp"

int main()
{
    // Keep the newest four horizontal-accuracy readings (millimetres).
    castle::statistic::sample_window<uint32_t, 4U> window;
    CASTLE_SAMPLE_CHECK(window.empty());

    CASTLE_SAMPLE_CHECK(!window.push(1200U));
    CASTLE_SAMPLE_CHECK(!window.push(1500U));
    CASTLE_SAMPLE_CHECK(!window.push(1100U));
    CASTLE_SAMPLE_CHECK(!window.push(1800U));
    CASTLE_SAMPLE_CHECK(window.full());

    // A fifth reading evicts the oldest; the evicted value is reported so an
    // incremental estimator could retract it.
    uint32_t evicted = 0U;
    CASTLE_SAMPLE_CHECK(window.push(900U, evicted));
    CASTLE_SAMPLE_CHECK(evicted == 1200U);

    // Samples are addressed oldest to newest: 1500 1100 1800 900.
    uint32_t value = 0U;
    CASTLE_SAMPLE_CHECK(window.oldest(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(value == 1500U);
    CASTLE_SAMPLE_CHECK(window.newest(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(value == 900U);
    CASTLE_SAMPLE_CHECK(window.at(2U, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(value == 1800U);
    CASTLE_SAMPLE_CHECK(window.at(4U, value) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(window[9U] == 0U);

    // Sub-window statistic over the newest two samples: 1800 + 900.
    uint32_t recent_sum = 0U;
    for (const uint32_t sample : window.latest(2U))
    {
        recent_sum += sample;
    }
    CASTLE_SAMPLE_CHECK(recent_sum == 2700U);

    // Copy the whole window into caller-owned storage.
    uint32_t copy[4] = {0U, 0U, 0U, 0U};
    CASTLE_SAMPLE_CHECK(window.copy_to(copy, 4U) == 4U);
    CASTLE_SAMPLE_CHECK(copy[0] == 1500U);
    CASTLE_SAMPLE_CHECK(copy[3] == 900U);

    window.clear();
    CASTLE_SAMPLE_CHECK(window.empty());
    CASTLE_SAMPLE_CHECK(window.available() == 4U);

    return 0;
}
