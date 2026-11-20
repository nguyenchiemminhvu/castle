#include <gtest/gtest.h>

#include "castle/dsa/count_min_sketch.hpp"
#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::dsa::count_min_depth;
using castle::dsa::count_min_sketch;
using castle::dsa::count_min_sketch_for;
using castle::dsa::count_min_update;
using castle::dsa::count_min_width;

// Weak hasher: identity plus a seed. Row 0 then uses column `(key + seed) mod Width`, which lets
// tests force collisions, and different seeds change the counter layout.
struct seeded_hash
{
    size_type seed;

    size_type operator()(const uint32_t& key) const noexcept
    {
        return static_cast<size_type>(key) + seed;
    }
};

// Deterministic skewed stream: key k (0 .. 63) occurs 64 / (k + 1) times, at least once.
constexpr uint32_t stream_keys = 64U;

uint32_t true_count(uint32_t key)
{
    return 64U / (key + 1U);
}

template <typename Sketch>
void feed_stream(Sketch& sketch)
{
    for (uint32_t key = 0U; key < stream_keys; ++key)
    {
        for (uint32_t i = 0U; i < true_count(key); ++i)
        {
            sketch.add(key * 2654435761U);
        }
    }
}

uint64_t stream_total()
{
    uint64_t total = 0U;
    for (uint32_t key = 0U; key < stream_keys; ++key)
    {
        total += true_count(key);
    }
    return total;
}

TEST(CountMinSketchTest, SizingFunctions)
{
    static_assert(count_min_width(100U) == 272U, "ceil(e * 100)");
    static_assert(count_min_width(1000U) == 2719U, "ceil(e * 1000)");
    static_assert(count_min_depth(2U) == 5U, "99 %: ceil(2 * ln 10)");
    static_assert(count_min_depth(0U) == 1U, "zero nines");

    volatile size_type inverse_epsilon = 10U;
    volatile size_type nines = 3U;
    EXPECT_EQ(count_min_width(inverse_epsilon), 28U); // 27.18
    EXPECT_EQ(count_min_width(1U), 3U);               // 2.72
    EXPECT_EQ(count_min_width(0U), 1U);               // clamped to one
    EXPECT_EQ(count_min_depth(nines), 7U);            // 6.91
    EXPECT_EQ(count_min_depth(1U), 3U);               // 2.30
    EXPECT_EQ(count_min_depth(4U), 10U);              // 9.21
    EXPECT_EQ(count_min_depth(0U), 1U);               // clamped to one
}

TEST(CountMinSketchTest, SizedAliasPicksWidthAndDepth)
{
    using sketch_type = count_min_sketch_for<uint32_t, 100U>;
    static_assert(sketch_type::width() == 272U, "ceil(e * 100)");
    static_assert(sketch_type::depth() == 5U, "two nines");
    static_assert(sketch_type::update_rule == count_min_update::standard, "default rule");

    using explicit_type = count_min_sketch<uint32_t, 272U, 5U>;
    static_assert(sizeof(sketch_type) == sizeof(explicit_type), "same layout");

    using small_type = count_min_sketch_for<uint32_t, 20U, 1U, uint8_t, count_min_update::conservative>;
    static_assert(small_type::width() == 55U, "ceil(e * 20)");
    static_assert(small_type::depth() == 3U, "one nine");
    static_assert(small_type::update_rule == count_min_update::conservative, "selected rule");
    EXPECT_EQ(small_type::counter_count(), 165U);
    EXPECT_EQ(small_type::max_count(), 255U);
}

TEST(CountMinSketchTest, ErrorParameters)
{
    using sketch_type = count_min_sketch<uint32_t, 272U, 5U>;
    EXPECT_NEAR(sketch_type::epsilon(), 0.01F, 0.0001F);  // e / 272
    EXPECT_NEAR(sketch_type::delta(), 0.006738F, 0.00001F); // e^-5
    EXPECT_EQ(sketch_type::max_count(), 0xFFFFFFFFU);

    sketch_type sketch;
    EXPECT_FLOAT_EQ(sketch.error_bound(), 0.0F);
    sketch.add(1U, 1000U);
    EXPECT_NEAR(sketch.error_bound(), 9.9937F, 0.001F); // 1000 * e / 272
}

TEST(CountMinSketchTest, StartsEmpty)
{
    count_min_sketch<uint32_t, 64U, 4U> sketch;
    EXPECT_TRUE(sketch.empty());
    EXPECT_EQ(sketch.total(), 0U);
    for (uint32_t key = 0U; key < 100U; ++key)
    {
        EXPECT_EQ(sketch.estimate(key), 0U);
    }
    for (const uint32_t cell : sketch.counters())
    {
        EXPECT_EQ(cell, 0U);
    }
}

TEST(CountMinSketchTest, AddReturnsEstimate)
{
    count_min_sketch<uint32_t, 1024U, 4U> sketch;
    EXPECT_EQ(sketch.add(42U), 1U);
    EXPECT_EQ(sketch.add(42U), 2U);
    EXPECT_EQ(sketch.add(42U, 5U), 7U);
    EXPECT_EQ(sketch.add(7U, 3U), 3U);
    EXPECT_EQ(sketch.estimate(42U), 7U);
    EXPECT_EQ(sketch.estimate(7U), 3U);
    EXPECT_EQ(sketch.total(), 10U);
    EXPECT_FALSE(sketch.empty());

    // A zero count changes nothing and still reports the estimate.
    EXPECT_EQ(sketch.add(42U, 0U), 7U);
    EXPECT_EQ(sketch.total(), 10U);

    // Standard update: every row of a key holds the same count when nothing collides.
    size_type rows_with_seven = 0U;
    for (const uint32_t cell : sketch.counters())
    {
        if (cell == 7U)
        {
            ++rows_with_seven;
        }
    }
    EXPECT_EQ(rows_with_seven, 4U);
}

TEST(CountMinSketchTest, NeverUndercountsPowerOfTwoWidth)
{
    count_min_sketch<uint32_t, 64U, 4U> sketch; // mask path
    feed_stream(sketch);
    EXPECT_EQ(sketch.total(), stream_total());
    for (uint32_t key = 0U; key < stream_keys; ++key)
    {
        EXPECT_GE(sketch.estimate(key * 2654435761U), true_count(key));
    }
}

TEST(CountMinSketchTest, NeverUndercountsOddWidth)
{
    count_min_sketch<uint32_t, 61U, 4U> sketch; // prime width: modulo path
    feed_stream(sketch);
    for (uint32_t key = 0U; key < stream_keys; ++key)
    {
        EXPECT_GE(sketch.estimate(key * 2654435761U), true_count(key));
    }
}

TEST(CountMinSketchTest, ErrorStaysWithinBound)
{
    // epsilon = 1 / 20, delta = 0.67 %: almost every key must stay within epsilon * N.
    count_min_sketch_for<uint32_t, 20U> sketch;
    feed_stream(sketch);
    const uint64_t bound = static_cast<uint64_t>(sketch.error_bound());
    EXPECT_GT(bound, 0U);

    size_type within = 0U;
    for (uint32_t key = 0U; key < stream_keys; ++key)
    {
        const uint32_t estimate = sketch.estimate(key * 2654435761U);
        if (estimate <= true_count(key) + bound)
        {
            ++within;
        }
    }
    EXPECT_GE(within, stream_keys - 1U);

    // Keys never added are estimated from collisions only.
    for (uint32_t key = 1000000U; key < 1000100U; ++key)
    {
        EXPECT_LE(sketch.estimate(key), bound + 1U);
    }
}

TEST(CountMinSketchTest, ConservativeUpdateOverestimatesLess)
{
    // Small width forces collisions.
    count_min_sketch<uint32_t, 16U, 3U> standard;
    count_min_sketch<uint32_t, 16U, 3U, uint32_t, count_min_update::conservative> conservative;
    feed_stream(standard);
    feed_stream(conservative);
    EXPECT_EQ(standard.total(), conservative.total());

    uint64_t standard_excess = 0U;
    uint64_t conservative_excess = 0U;
    for (uint32_t key = 0U; key < stream_keys; ++key)
    {
        const uint32_t exact = true_count(key);
        const uint32_t loose = standard.estimate(key * 2654435761U);
        const uint32_t tight = conservative.estimate(key * 2654435761U);
        EXPECT_GE(tight, exact);
        EXPECT_LE(tight, loose);
        standard_excess += loose - exact;
        conservative_excess += tight - exact;
    }
    EXPECT_LT(conservative_excess, standard_excess);
}

TEST(CountMinSketchTest, ConservativeUpdateSkipsHigherCounters)
{
    // Keys 1 and 9 share row 0 (identity hash, width 8). Row 1 differs: the steps differ.
    using sketch_type = count_min_sketch<uint32_t, 8U, 2U, uint32_t, count_min_update::conservative, seeded_hash>;
    sketch_type sketch(seeded_hash{0U});
    EXPECT_EQ(sketch.add(1U, 10U), 10U);
    EXPECT_EQ(sketch.add(9U, 1U), 1U); // row 0 already holds 10: left unchanged
    EXPECT_EQ(sketch.counters()(0U, 1U), 10U);
    EXPECT_EQ(sketch.estimate(1U), 10U);
    EXPECT_EQ(sketch.estimate(9U), 1U);

    // Standard update adds to the shared counter.
    count_min_sketch<uint32_t, 8U, 2U, uint32_t, count_min_update::standard, seeded_hash> plain(seeded_hash{0U});
    plain.add(1U, 10U);
    EXPECT_EQ(plain.add(9U, 1U), 1U);
    EXPECT_EQ(plain.counters()(0U, 1U), 11U);
}

TEST(CountMinSketchTest, CountersSaturate)
{
    count_min_sketch<uint32_t, 32U, 2U, uint8_t> standard;
    EXPECT_EQ(standard.add(5U, 200U), 200U);
    EXPECT_EQ(standard.add(5U, 100U), 255U);
    EXPECT_EQ(standard.estimate(5U), 255U);
    EXPECT_EQ(standard.total(), 300U); // the total keeps counting past the counter limit

    count_min_sketch<uint32_t, 32U, 2U, uint8_t, count_min_update::conservative> conservative;
    EXPECT_EQ(conservative.add(5U, 200U), 200U);
    EXPECT_EQ(conservative.add(5U, 100U), 255U);
    EXPECT_EQ(conservative.estimate(5U), 255U);

    count_min_sketch<uint32_t, 32U, 2U, uint64_t> wide;
    wide.add(1U, 0xFFFFFFFFFFFFFFF0ULL);
    wide.add(1U, 0x100U);
    EXPECT_EQ(wide.total(), 0xFFFFFFFFFFFFFFFFULL); // total saturates too
    EXPECT_EQ(wide.estimate(1U), 0xFFFFFFFFFFFFFFFFULL);
}

TEST(CountMinSketchTest, DepthEqualsWidth)
{
    // Every probe step is used: the column sequence stays inside the row.
    count_min_sketch<uint32_t, 3U, 3U> sketch;
    for (uint32_t key = 0U; key < 50U; ++key)
    {
        sketch.add(key, key + 1U);
    }
    for (uint32_t key = 0U; key < 50U; ++key)
    {
        EXPECT_GE(sketch.estimate(key), key + 1U);
    }
}

TEST(CountMinSketchTest, MergeIsSumOfStreams)
{
    count_min_sketch<uint32_t, 128U, 4U> left;
    count_min_sketch<uint32_t, 128U, 4U> right;
    count_min_sketch<uint32_t, 128U, 4U> both;
    for (uint32_t key = 0U; key < 40U; ++key)
    {
        left.add(key, 2U);
        both.add(key, 2U);
    }
    for (uint32_t key = 20U; key < 60U; ++key)
    {
        right.add(key, 3U);
        both.add(key, 3U);
    }

    left.merge(right);
    EXPECT_TRUE(left == both); // standard update is linear
    EXPECT_EQ(left.total(), 200U);
    EXPECT_GE(left.estimate(10U), 2U);
    EXPECT_GE(left.estimate(30U), 5U);
    EXPECT_GE(left.estimate(50U), 3U);
}

TEST(CountMinSketchTest, MergeConservativeAndSaturating)
{
    using sketch_type = count_min_sketch<uint32_t, 64U, 3U, uint8_t, count_min_update::conservative>;
    sketch_type left;
    sketch_type right;
    left.add(1U, 200U);
    right.add(1U, 100U);
    right.add(2U, 7U);

    left.merge(right);
    EXPECT_EQ(left.estimate(1U), 255U); // saturated sum
    EXPECT_GE(left.estimate(2U), 7U);
    EXPECT_EQ(left.total(), 307U);
}

TEST(CountMinSketchTest, HalveAgesCounts)
{
    count_min_sketch<uint32_t, 256U, 4U> sketch;
    sketch.add(1U, 9U);
    sketch.add(2U, 4U);
    sketch.add(3U, 1U);

    sketch.halve();
    EXPECT_EQ(sketch.estimate(1U), 4U);
    EXPECT_EQ(sketch.estimate(2U), 2U);
    EXPECT_EQ(sketch.estimate(3U), 0U);
    EXPECT_EQ(sketch.total(), 7U);

    sketch.halve();
    sketch.halve();
    sketch.halve();
    EXPECT_EQ(sketch.estimate(1U), 0U);
    EXPECT_EQ(sketch.total(), 0U);
    EXPECT_TRUE(sketch.empty());
}

TEST(CountMinSketchTest, ClearCopyAndEquality)
{
    count_min_sketch<uint32_t, 32U, 2U> sketch;
    sketch.add(11U, 4U);

    count_min_sketch<uint32_t, 32U, 2U> copy(sketch);
    EXPECT_TRUE(copy == sketch);
    EXPECT_FALSE(copy != sketch);
    EXPECT_EQ(copy.estimate(11U), 4U);

    // Same counters, different total.
    count_min_sketch<uint32_t, 32U, 2U> other_total;
    ASSERT_EQ(other_total.load_counters(sketch.counters().data(), sketch.counter_count(), 5U), castle::status::ok);
    EXPECT_FALSE(other_total == sketch);
    EXPECT_TRUE(other_total != sketch);

    // Same total, different counters.
    count_min_sketch<uint32_t, 32U, 2U> other_key;
    other_key.add(12U, 4U);
    EXPECT_EQ(other_key.total(), sketch.total());
    EXPECT_TRUE(other_key != sketch);

    sketch.clear();
    EXPECT_TRUE(sketch.empty());
    EXPECT_EQ(sketch.estimate(11U), 0U);
    EXPECT_TRUE((sketch == count_min_sketch<uint32_t, 32U, 2U>()));
    EXPECT_EQ(copy.estimate(11U), 4U); // the copy is independent
}

TEST(CountMinSketchTest, SeededHasherChangesLayout)
{
    using sketch_type = count_min_sketch<uint32_t, 16U, 2U, uint32_t, count_min_update::standard, seeded_hash>;
    sketch_type first(seeded_hash{0U});
    sketch_type second(seeded_hash{5U});
    EXPECT_EQ(first.hash_function().seed, 0U);
    EXPECT_EQ(second.hash_function().seed, 5U);

    first.add(3U);
    second.add(3U);
    EXPECT_EQ(first.counters()(0U, 3U), 1U);  // column (3 + 0) mod 16
    EXPECT_EQ(second.counters()(0U, 8U), 1U); // column (3 + 5) mod 16
    EXPECT_TRUE(first != second);
    EXPECT_EQ(first.estimate(3U), 1U);
    EXPECT_EQ(second.estimate(3U), 1U);
}

TEST(CountMinSketchTest, StringViewKeys)
{
    using castle::dsa::string_view;
    count_min_sketch<string_view, 64U, 3U, uint16_t> sketch;
    sketch.add(string_view("door_open"));
    sketch.add(string_view("door_open"));
    sketch.add(string_view("low_battery"));
    EXPECT_EQ(sketch.estimate(string_view("door_open")), 2U);
    EXPECT_EQ(sketch.estimate(string_view("low_battery")), 1U);
    EXPECT_EQ(sketch.estimate(string_view("overheat")), 0U);
}

TEST(CountMinSketchTest, LoadCountersRoundTripAndValidation)
{
    using sketch_type = count_min_sketch<uint32_t, 16U, 3U, uint16_t>;
    sketch_type source;
    source.add(100U, 6U);
    source.add(200U, 2U);

    uint16_t saved[sketch_type::counter_count()] = {};
    for (size_type i = 0U; i < sketch_type::counter_count(); ++i)
    {
        saved[i] = source.counters().data()[i];
    }

    sketch_type restored;
    restored.add(7U); // previous content, kept on failure
    const sketch_type before(restored);
    EXPECT_EQ(restored.load_counters(nullptr, sketch_type::counter_count(), 8U), castle::status::invalid_argument);
    EXPECT_TRUE(restored == before);
    EXPECT_EQ(restored.load_counters(saved, sketch_type::counter_count() - 1U, 8U), castle::status::out_of_range);
    EXPECT_TRUE(restored == before);

    EXPECT_EQ(restored.load_counters(saved, sketch_type::counter_count(), source.total()), castle::status::ok);
    EXPECT_TRUE(restored == source);
    EXPECT_EQ(restored.estimate(100U), source.estimate(100U));
    EXPECT_EQ(restored.estimate(200U), source.estimate(200U));
    EXPECT_EQ(restored.total(), 8U);
}
} // namespace
