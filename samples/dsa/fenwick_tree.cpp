#include "sample_support.hpp"

#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/fenwick_tree.hpp"

namespace
{
constexpr castle::size_type k_max_nums = 16U;

// LeetCode 327 (Count of Range Sum): count pairs i < j of prefix sums with
// lower <= prefix[j] - prefix[i] <= upper, i.e. prefix[j] - upper <= prefix[i] <= prefix[j] - lower.
// Prefix sums are compressed to ranks; the Fenwick tree counts how many earlier prefixes hold each rank.
int32_t count_range_sum(const int32_t* nums, castle::size_type count, int32_t lower, int32_t upper)
{
    CASTLE_SAMPLE_CHECK(count <= k_max_nums);

    int64_t prefix[k_max_nums + 1U] = {};
    for (castle::size_type i = 0U; i < count; ++i)
    {
        prefix[i + 1U] = prefix[i] + nums[i]; // 64-bit: the sum of 32-bit values may overflow 32 bits
    }

    int64_t ranks[k_max_nums + 1U] = {};
    for (castle::size_type i = 0U; i <= count; ++i)
    {
        ranks[i] = prefix[i];
    }
    castle::sort(ranks, ranks + count + 1U);
    int64_t* const ranks_end = castle::unique(ranks, ranks + count + 1U);

    castle::dsa::fenwick_tree<uint32_t, k_max_nums + 1U> seen(static_cast<castle::size_type>(ranks_end - ranks));
    int32_t result = 0;
    for (castle::size_type j = 0U; j <= count; ++j)
    {
        const castle::size_type first = static_cast<castle::size_type>(
            castle::lower_bound(ranks, ranks_end, prefix[j] - upper) - ranks);
        const castle::size_type last = static_cast<castle::size_type>(
            castle::upper_bound(ranks, ranks_end, prefix[j] - lower) - ranks);
        result += static_cast<int32_t>(seen.prefix_sum(last) - seen.prefix_sum(first));

        const castle::size_type rank = static_cast<castle::size_type>(
            castle::lower_bound(ranks, ranks_end, prefix[j]) - ranks);
        CASTLE_SAMPLE_CHECK(seen.add(rank, 1U) == castle::status::ok);
    }
    return result;
}
} // namespace

int main()
{
    // Example 1: nums = [-2, 5, -1], lower = -2, upper = 2 -> 3 ([0,0], [2,2], [0,2]).
    const int32_t example1[] = {-2, 5, -1};
    CASTLE_SAMPLE_CHECK(count_range_sum(example1, 3U, -2, 2) == 3);

    // Example 2: nums = [0], lower = 0, upper = 0 -> 1.
    const int32_t example2[] = {0};
    CASTLE_SAMPLE_CHECK(count_range_sum(example2, 1U, 0, 0) == 1);

    // Extreme values: the prefix sums leave the 32-bit range but the counts stay exact.
    const int32_t extremes[] = {2147483647, 2147483647, -2147483647 - 1};
    CASTLE_SAMPLE_CHECK(count_range_sum(extremes, 3U, -1, 0) == 1); // [1,2] sums to -1

    // Running median of 8-bit sensor bins: a frequency table answers order statistics.
    // lower_bound(k) returns the smallest bin whose cumulative count reaches k.
    using histogram_type = castle::dsa::fenwick_tree<uint16_t, 16U>;
    histogram_type histogram(16U);
    const uint8_t readings[] = {7U, 3U, 12U, 7U, 9U};
    for (const uint8_t bin : readings)
    {
        CASTLE_SAMPLE_CHECK(histogram.add(bin, 1U) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(histogram.prefix_sum(16U) == 5U);
    CASTLE_SAMPLE_CHECK(histogram.lower_bound(3U) == 7U);  // median of {3, 7, 7, 9, 12}
    CASTLE_SAMPLE_CHECK(histogram.lower_bound(1U) == 3U);  // minimum
    CASTLE_SAMPLE_CHECK(histogram.lower_bound(6U) == histogram_type::npos); // only 5 readings

    // A reading is recalibrated: bin 12 becomes bin 4.
    CASTLE_SAMPLE_CHECK(histogram.add(12U, static_cast<uint16_t>(-1)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(histogram.add(4U, 1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(histogram.range_sum(0U, 7U) == 4U);
    CASTLE_SAMPLE_CHECK(histogram.get(7U) == 2U);

    return 0;
}
