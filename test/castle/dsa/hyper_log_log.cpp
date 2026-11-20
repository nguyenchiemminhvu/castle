#include <gtest/gtest.h>

#include "castle/dsa/hyper_log_log.hpp"
#include "castle/dsa/string_view.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;
using castle::dsa::hyper_log_log;

// Hasher with a seed: different seeds give independent register layouts.
struct seeded_hash
{
    size_type seed;

    size_type operator()(const uint32_t& key) const noexcept
    {
        return static_cast<size_type>(castle::detail::hash_mix64(static_cast<uint64_t>(key) + (static_cast<uint64_t>(seed) << 32U)));
    }
};

uint64_t spread(uint64_t index)
{
    return index * 0x9E3779B97F4A7C15ULL + 77U;
}

template <typename Sketch>
void add_range(Sketch& sketch, uint64_t first, uint64_t last)
{
    for (uint64_t i = first; i < last; ++i)
    {
        sketch.add(spread(i));
    }
}

double relative_error(uint64_t estimate, uint64_t truth)
{
    const double diff = static_cast<double>(estimate) - static_cast<double>(truth);
    return ((diff < 0.0) ? -diff : diff) / static_cast<double>(truth);
}

TEST(HyperLogLogTest, Geometry)
{
    using default_type = hyper_log_log<uint64_t>;
    static_assert(default_type::precision() == 10U, "default precision");
    static_assert(default_type::register_count() == 1024U, "2^10 registers");
    static_assert(default_type::max_rank() == 55U, "64 - 10 + 1");
    static_assert(sizeof(default_type::register_array) == 1024U, "one byte per register");

    EXPECT_EQ((hyper_log_log<uint32_t, 4U>::register_count()), 16U);
    EXPECT_EQ((hyper_log_log<uint32_t, 4U>::max_rank()), 61U);
    EXPECT_EQ((hyper_log_log<uint32_t, 16U>::register_count()), 65536U);
    EXPECT_EQ((hyper_log_log<uint32_t, 16U>::max_rank()), 49U);
}

TEST(HyperLogLogTest, StandardError)
{
    EXPECT_NEAR((hyper_log_log<uint32_t, 4U>::standard_error()), 0.26F, 1e-6F);     // 1.04 / 4
    EXPECT_NEAR((hyper_log_log<uint32_t, 10U>::standard_error()), 0.0325F, 1e-6F);  // 1.04 / 32
    EXPECT_NEAR((hyper_log_log<uint32_t, 11U>::standard_error()), 0.02298F, 1e-5F); // 1.04 / 45.25
    EXPECT_NEAR((hyper_log_log<uint32_t, 16U>::standard_error()), 0.0040625F, 1e-7F);
}

TEST(HyperLogLogTest, StartsEmpty)
{
    hyper_log_log<uint64_t> sketch;
    EXPECT_TRUE(sketch.empty());
    EXPECT_EQ(sketch.estimate(), 0U);
    for (const uint8_t value : sketch.registers())
    {
        EXPECT_EQ(value, 0U);
    }
}

TEST(HyperLogLogTest, DuplicatesDoNotCount)
{
    hyper_log_log<uint64_t> sketch;
    EXPECT_TRUE(sketch.add(42U)); // the first sighting raises a register
    for (uint32_t i = 0U; i < 1000U; ++i)
    {
        EXPECT_FALSE(sketch.add(42U));
    }
    EXPECT_EQ(sketch.estimate(), 1U);
    EXPECT_FALSE(sketch.empty());
}

TEST(HyperLogLogTest, SmallCardinalitiesAreNearlyExact)
{
    hyper_log_log<uint64_t> sketch;
    for (uint64_t n = 1U; n <= 64U; ++n)
    {
        sketch.add(spread(n));
        const uint64_t estimate = sketch.estimate();
        EXPECT_GE(estimate + 2U, n);
        EXPECT_LE(estimate, n + 2U);
    }
}

TEST(HyperLogLogTest, LargeCardinalitiesStayWithinTheStandardError)
{
    hyper_log_log<uint64_t> sketch;
    const double sigma = static_cast<double>(hyper_log_log<uint64_t>::standard_error());
    const uint64_t checkpoints[] = {500U, 2000U, 10000U, 50000U, 200000U};
    uint64_t added = 0U;
    for (const uint64_t target : checkpoints)
    {
        add_range(sketch, added, target);
        added = target;
        EXPECT_LT(relative_error(sketch.estimate(), target), 3.0 * sigma) << "n = " << target;
    }

    // Adding the same keys again changes nothing.
    const hyper_log_log<uint64_t> before = sketch;
    add_range(sketch, 0U, added);
    EXPECT_TRUE(sketch == before);
}

TEST(HyperLogLogTest, HigherPrecisionIsMoreAccurate)
{
    hyper_log_log<uint64_t, 6U> coarse;
    hyper_log_log<uint64_t, 14U> fine;
    add_range(coarse, 0U, 100000U);
    add_range(fine, 0U, 100000U);
    EXPECT_LT(relative_error(coarse.estimate(), 100000U), 3.0 * static_cast<double>(hyper_log_log<uint64_t, 6U>::standard_error()));
    EXPECT_LT(relative_error(fine.estimate(), 100000U), 3.0 * static_cast<double>(hyper_log_log<uint64_t, 14U>::standard_error()));
}

TEST(HyperLogLogTest, MergeEqualsTheUnionStream)
{
    hyper_log_log<uint64_t> first;
    hyper_log_log<uint64_t> second;
    hyper_log_log<uint64_t> both;
    add_range(first, 0U, 6000U);
    add_range(second, 4000U, 10000U); // 2000 keys overlap
    add_range(both, 0U, 10000U);

    hyper_log_log<uint64_t> merged = first;
    merged.merge(second);
    EXPECT_TRUE(merged == both);
    EXPECT_LT(relative_error(merged.estimate(), 10000U), 0.1);

    // Merge is commutative and idempotent.
    hyper_log_log<uint64_t> reversed = second;
    reversed.merge(first);
    EXPECT_TRUE(reversed == merged);
    reversed.merge(first);
    EXPECT_TRUE(reversed == merged);

    // Merging an empty sketch changes nothing.
    merged.merge(hyper_log_log<uint64_t>());
    EXPECT_TRUE(merged == both);
}

TEST(HyperLogLogTest, LoadRegistersValidatesInput)
{
    using sketch_type = hyper_log_log<uint64_t, 4U>;
    sketch_type source;
    add_range(source, 0U, 30U);

    sketch_type restored;
    EXPECT_EQ(restored.load_registers(nullptr, 16U), status::invalid_argument);
    EXPECT_EQ(restored.load_registers(source.registers().data(), 15U), status::out_of_range);

    uint8_t corrupt[16] = {};
    corrupt[7] = static_cast<uint8_t>(sketch_type::max_rank() + 1U);
    EXPECT_EQ(restored.load_registers(corrupt, 16U), status::invalid_argument);
    EXPECT_TRUE(restored.empty()); // unchanged on failure

    EXPECT_EQ(restored.load_registers(source.registers().data(), 16U), status::ok);
    EXPECT_TRUE(restored == source);
    EXPECT_EQ(restored.estimate(), source.estimate());
}

TEST(HyperLogLogTest, EstimateGrowsWithTheRegisters)
{
    using sketch_type = hyper_log_log<uint64_t, 4U>;
    uint8_t registers[16] = {};
    sketch_type sketch;
    uint64_t previous = 0U;
    // Raise the registers one step at a time up to saturation: the estimate never decreases.
    for (uint8_t rank = 1U; rank <= sketch_type::max_rank(); ++rank)
    {
        for (size_type i = 0U; i < 16U; ++i)
        {
            registers[i] = rank;
            ASSERT_EQ(sketch.load_registers(registers, 16U), status::ok);
            const uint64_t estimate = sketch.estimate();
            EXPECT_GE(estimate, previous) << "rank " << static_cast<unsigned>(rank) << ", register " << i;
            previous = estimate;
        }
    }
}

TEST(HyperLogLogTest, SaturatedRegistersGiveTheMaximum)
{
    using sketch_type = hyper_log_log<uint64_t, 4U>;
    uint8_t registers[16];
    for (uint8_t& value : registers)
    {
        value = sketch_type::max_rank();
    }
    sketch_type sketch;
    ASSERT_EQ(sketch.load_registers(registers, 16U), status::ok);
    EXPECT_EQ(sketch.estimate(), UINT64_MAX); // every register saturated: infinite estimate

    registers[0] = static_cast<uint8_t>(sketch_type::max_rank() - 1U);
    ASSERT_EQ(sketch.load_registers(registers, 16U), status::ok);
    EXPECT_EQ(sketch.estimate(), UINT64_MAX); // beyond 2^64

    // Half the registers saturated, half at rank 1: finite, about 4 * alpha * m.
    for (size_type i = 0U; i < 16U; ++i)
    {
        registers[i] = (i < 8U) ? sketch_type::max_rank() : 1U;
    }
    ASSERT_EQ(sketch.load_registers(registers, 16U), status::ok);
    EXPECT_NEAR(static_cast<double>(sketch.estimate()), 46.0, 1.0);
}

TEST(HyperLogLogTest, ClearAndEquality)
{
    hyper_log_log<uint64_t, 8U> lhs;
    hyper_log_log<uint64_t, 8U> rhs;
    EXPECT_TRUE(lhs == rhs);
    EXPECT_FALSE(lhs != rhs);

    lhs.add(1U);
    EXPECT_TRUE(lhs != rhs);
    rhs.add(1U);
    EXPECT_TRUE(lhs == rhs);

    lhs.clear();
    EXPECT_TRUE(lhs.empty());
    EXPECT_EQ(lhs.estimate(), 0U);
    EXPECT_TRUE(lhs != rhs);
}

TEST(HyperLogLogTest, SeededHashersChangeTheLayout)
{
    using sketch_type = hyper_log_log<uint32_t, 8U, seeded_hash>;
    sketch_type lhs(seeded_hash{1U});
    sketch_type rhs(seeded_hash{2U});
    EXPECT_EQ(lhs.hash_function().seed, 1U);
    EXPECT_EQ(rhs.hash_function().seed, 2U);
    for (uint32_t i = 0U; i < 300U; ++i)
    {
        lhs.add(i);
        rhs.add(i);
    }
    EXPECT_TRUE(lhs != rhs);
    EXPECT_LT(relative_error(lhs.estimate(), 300U), 0.25);
    EXPECT_LT(relative_error(rhs.estimate(), 300U), 0.25);
}

TEST(HyperLogLogTest, StringViewKeys)
{
    using castle::dsa::string_view;
    hyper_log_log<string_view, 6U> codes;
    codes.add(string_view("P0300"));
    codes.add(string_view("U0100"));
    codes.add(string_view("P0300"));
    codes.add(string_view("C1234"));
    EXPECT_EQ(codes.estimate(), 3U);
}

} // namespace
