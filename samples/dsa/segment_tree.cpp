#include "sample_support.hpp"

#include "castle/dsa/segment_tree.hpp"

namespace
{
// LeetCode 307 (Range Sum Query - Mutable), sized for its limit of 3 * 10^4 elements.
constexpr castle::size_type k_max_nums = 30000U;

class num_array
{
public:
    num_array(const int32_t* nums, castle::size_type count)
    {
        CASTLE_SAMPLE_CHECK(sums_.build(nums, count) == castle::status::ok);
    }

    void update(castle::size_type index, int32_t val)
    {
        CASTLE_SAMPLE_CHECK(sums_.set(index, val) == castle::status::ok);
    }

    int32_t sum_range(castle::size_type left, castle::size_type right) const
    {
        return sums_.query(left, right);
    }

private:
    castle::dsa::segment_tree<castle::dsa::sum_monoid<int32_t>, k_max_nums> sums_;
};

// A custom monoid: the best sum of a non-empty contiguous subarray (LeetCode 53, with updates).
// combine() is associative but not commutative; the tree always combines left to right.
struct span_score
{
    int32_t total;
    int32_t best_prefix;
    int32_t best_suffix;
    int32_t best;
};

constexpr int32_t k_minus_infinity = -1000000000; // far below any reachable sum, yet safe to add twice

int32_t max_of(int32_t lhs, int32_t rhs)
{
    return (lhs < rhs) ? rhs : lhs;
}

struct max_subarray_monoid
{
    using value_type = span_score;

    static span_score identity()
    {
        return span_score{0, k_minus_infinity, k_minus_infinity, k_minus_infinity};
    }

    static span_score combine(const span_score& lhs, const span_score& rhs)
    {
        return span_score{lhs.total + rhs.total,
                          max_of(lhs.best_prefix, lhs.total + rhs.best_prefix),
                          max_of(rhs.best_suffix, rhs.total + lhs.best_suffix),
                          max_of(max_of(lhs.best, rhs.best), lhs.best_suffix + rhs.best_prefix)};
    }

    static span_score leaf(int32_t value)
    {
        return span_score{value, value, value, value};
    }
};
} // namespace

int main()
{
    // LeetCode 307 example: [1, 3, 5]; sumRange(0, 2) = 9; update(1, 2); sumRange(0, 2) = 8.
    {
        const int32_t nums[] = {1, 3, 5};
        static num_array leet(nums, 3U); // static: the fixed-capacity tree is 256 KiB
        CASTLE_SAMPLE_CHECK(leet.sum_range(0U, 2U) == 9);
        leet.update(1U, 2);
        CASTLE_SAMPLE_CHECK(leet.sum_range(0U, 2U) == 8);
        CASTLE_SAMPLE_CHECK(leet.sum_range(1U, 2U) == 7);
    }

    // Maximum subarray sum while elements change.
    {
        const int32_t nums[] = {-2, 1, -3, 4, -1, 2, 1, -5, 4};
        castle::dsa::segment_tree<max_subarray_monoid, 16U> spans;
        CASTLE_SAMPLE_CHECK(spans.assign(9U) == castle::status::ok);
        for (castle::size_type i = 0U; i < 9U; ++i)
        {
            CASTLE_SAMPLE_CHECK(spans.set(i, max_subarray_monoid::leaf(nums[i])) == castle::status::ok);
        }
        CASTLE_SAMPLE_CHECK(spans.all().best == 6);           // [4, -1, 2, 1]
        CASTLE_SAMPLE_CHECK(spans.query(0U, 2U).best == 1);   // [1]
        CASTLE_SAMPLE_CHECK(spans.set(7U, max_subarray_monoid::leaf(5)) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(spans.all().best == 15);          // [4, -1, 2, 1, 5, 4]
    }

    // Binary search on prefix sums: how many queued packets fit a 1500-byte frame?
    {
        const uint16_t packet_bytes[] = {400U, 300U, 500U, 200U, 600U};
        castle::dsa::segment_tree<castle::dsa::sum_monoid<uint16_t>, 8U> queue;
        CASTLE_SAMPLE_CHECK(queue.build(packet_bytes, 5U) == castle::status::ok);
        const castle::size_type fitting = queue.max_right(0U, [](uint16_t bytes) { return bytes <= 1500U; });
        CASTLE_SAMPLE_CHECK(fitting == 4U); // 400 + 300 + 500 + 200 = 1400; the fifth would exceed it
        // Packets taken from the back of the queue until 800 bytes are reached.
        CASTLE_SAMPLE_CHECK(queue.min_left(5U, [](uint16_t bytes) { return bytes <= 800U; }) == 3U);
    }

    // LeetCode 1109 (Corporate Flight Bookings): add seats to ranges of flights, then read each flight.
    {
        castle::dsa::lazy_segment_tree<castle::dsa::range_add_sum<int32_t>, 8U> seats;
        CASTLE_SAMPLE_CHECK(seats.assign(5U, 0) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(seats.apply(0U, 1U, 10) == castle::status::ok); // booking [1, 2, 10]
        CASTLE_SAMPLE_CHECK(seats.apply(1U, 2U, 20) == castle::status::ok); // booking [2, 3, 20]
        CASTLE_SAMPLE_CHECK(seats.apply(1U, 4U, 25) == castle::status::ok); // booking [2, 5, 25]
        const int32_t expected[] = {10, 55, 45, 25, 25};
        for (castle::size_type flight = 0U; flight < 5U; ++flight)
        {
            CASTLE_SAMPLE_CHECK(seats.get(flight) == expected[flight]);
        }
        CASTLE_SAMPLE_CHECK(seats.query(1U, 3U) == 125);
    }

    // LeetCode 732 (My Calendar III): the largest number of overlapping bookings after each one.
    {
        castle::dsa::lazy_segment_tree<castle::dsa::range_add_max<int32_t>, 64U> calendar;
        CASTLE_SAMPLE_CHECK(calendar.assign(60U, 0) == castle::status::ok);
        struct booking
        {
            castle::size_type start;
            castle::size_type end; // exclusive
            int32_t expected_overlap;
        };
        const booking bookings[] = {{10U, 20U, 1}, {50U, 60U, 1}, {10U, 40U, 2},
                                    {5U, 15U, 3},  {5U, 10U, 3},  {25U, 55U, 3}};
        for (const booking& next : bookings)
        {
            CASTLE_SAMPLE_CHECK(calendar.apply(next.start, next.end - 1U, 1) == castle::status::ok);
            CASTLE_SAMPLE_CHECK(calendar.all() == next.expected_overlap);
        }
    }

    // Range assignment: repaint a strip of LEDs and keep the lowest brightness of any segment.
    {
        castle::dsa::lazy_segment_tree<castle::dsa::range_assign_min<uint8_t>, 16U> strip;
        CASTLE_SAMPLE_CHECK(strip.assign(12U, 200U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(strip.apply(2U, 5U, 40U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(strip.apply(4U, 9U, 90U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(strip.query(0U, 3U) == 40U);
        CASTLE_SAMPLE_CHECK(strip.query(4U, 11U) == 90U);
        CASTLE_SAMPLE_CHECK(strip.get(10U) == 200U);
    }

    return 0;
}
