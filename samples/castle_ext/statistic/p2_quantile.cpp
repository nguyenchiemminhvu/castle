#include "sample_support.hpp"

#include <stdint.h>

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/p2_quantile.hpp"

namespace
{
// Small deterministic xorshift32 generator for a uniform test stream.
uint32_t next_random(uint32_t& state)
{
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}
} // namespace

int main()
{
    // Track the median and the 95th percentile of a stream with O(1) memory.
    castle::statistic::p2_quantile<double> median(0.5);
    castle::statistic::p2_quantile<double> p95(0.95);
    CASTLE_SAMPLE_CHECK(median.configured());
    CASTLE_SAMPLE_CHECK(p95.empty());

    double estimate = 0.0;
    CASTLE_SAMPLE_CHECK(p95.quantile(estimate) == castle::status::empty);

    // Fewer than five samples: the exact quantile of what was seen so far.
    CASTLE_SAMPLE_CHECK(median.push(3.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(median.push(1.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(median.push(2.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(median.quantile(estimate) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(estimate - 2.0) < 1e-12);
    median.clear();
    CASTLE_SAMPLE_CHECK(median.empty());

    // Uniform values in [0, 1000): the true median is 500 and the 95th percentile 950.
    uint32_t state = 2463534242U;
    for (uint32_t i = 0U; i < 100000U; ++i)
    {
        const double sample = static_cast<double>(next_random(state) >> 8U) * (1000.0 / 16777216.0);
        CASTLE_SAMPLE_CHECK(median.push(sample) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(p95.push(sample) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(p95.count() == 100000U);

    CASTLE_SAMPLE_CHECK(median.quantile(estimate) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(estimate - 500.0) < 5.0);
    CASTLE_SAMPLE_CHECK(p95.quantile(estimate) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(estimate - 950.0) < 5.0);

    // An invalid probability leaves the estimator unconfigured.
    castle::statistic::p2_quantile<double> invalid(1.0);
    CASTLE_SAMPLE_CHECK(!invalid.configured());
    CASTLE_SAMPLE_CHECK(invalid.push(1.0) == castle::status::not_configured);

    return 0;
}
