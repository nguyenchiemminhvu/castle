#include <gtest/gtest.h>

#include "castle/math/random.h"

#include <stdint.h>
#include <set>

namespace
{

// ---------------------------------------------------------------------------
// Determinism / reproducibility
// ---------------------------------------------------------------------------
TEST(MathRandom, SameSeedSameSequence)
{
    castle::math::random a(12345u);
    castle::math::random b(12345u);

    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(a.next(), b.next());
    }
}

TEST(MathRandom, DefaultConstructionIsDeterministic)
{
    castle::math::random a;
    castle::math::random b;

    for (int i = 0; i < 50; ++i)
    {
        EXPECT_EQ(a(), b());
    }
}

TEST(MathRandom, DifferentSeedsDiffer)
{
    castle::math::random a(1u);
    castle::math::random b(2u);

    bool any_different = false;
    for (int i = 0; i < 20; ++i)
    {
        if (a.next() != b.next())
        {
            any_different = true;
            break;
        }
    }
    EXPECT_TRUE(any_different);
}

TEST(MathRandom, ReseedResetsSequence)
{
    castle::math::random r(777u);
    const uint32_t first = r.next();
    const uint32_t second = r.next();

    r.seed(777u);
    EXPECT_EQ(r.next(), first);
    EXPECT_EQ(r.next(), second);
}

TEST(MathRandom, OperatorMatchesNext)
{
    castle::math::random a(42u);
    castle::math::random b(42u);
    EXPECT_EQ(a(), b.next());
}

// ---------------------------------------------------------------------------
// min / max bounds
// ---------------------------------------------------------------------------
TEST(MathRandom, MinMax)
{
    EXPECT_EQ(castle::math::random::min(), 0u);
    EXPECT_EQ(castle::math::random::max(), 0xFFFFFFFFu);
}

// ---------------------------------------------------------------------------
// uniform - values in [0, bound)
// ---------------------------------------------------------------------------
TEST(MathRandom, UniformWithinBound)
{
    castle::math::random r(99u);
    for (int i = 0; i < 10000; ++i)
    {
        const uint32_t v = r.uniform(10u);
        EXPECT_LT(v, 10u);
    }
}

TEST(MathRandom, UniformBoundOneAlwaysZero)
{
    castle::math::random r(3u);
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(r.uniform(1u), 0u);
    }
}

TEST(MathRandom, UniformCoversRange)
{
    castle::math::random r(2024u);
    std::set<uint32_t> seen;
    for (int i = 0; i < 1000; ++i)
    {
        seen.insert(r.uniform(6u));
    }
    // With 1000 draws over 6 buckets every value should appear.
    EXPECT_EQ(seen.size(), 6u);
}

// ---------------------------------------------------------------------------
// range - inclusive [low, high]
// ---------------------------------------------------------------------------
TEST(MathRandom, RangeWithinInclusiveBounds)
{
    castle::math::random r(55u);
    for (int i = 0; i < 10000; ++i)
    {
        const uint32_t v = r.range(100u, 200u);
        EXPECT_GE(v, 100u);
        EXPECT_LE(v, 200u);
    }
}

TEST(MathRandom, RangeSinglePoint)
{
    castle::math::random r(7u);
    for (int i = 0; i < 100; ++i)
    {
        EXPECT_EQ(r.range(42u, 42u), 42u);
    }
}

TEST(MathRandom, RangeFullDomain)
{
    castle::math::random a(123u);
    castle::math::random b(123u);
    // Full uint32_t domain must forward next() directly.
    EXPECT_EQ(a.range(0u, 0xFFFFFFFFu), b.next());
}

// ---------------------------------------------------------------------------
// discard - advances the stream
// ---------------------------------------------------------------------------
TEST(MathRandom, DiscardAdvances)
{
    castle::math::random a(500u);
    castle::math::random b(500u);

    a.discard(5);
    for (int i = 0; i < 5; ++i)
    {
        (void)b.next();
    }
    EXPECT_EQ(a.next(), b.next());
}

TEST(MathRandom, DiscardZeroIsNoOp)
{
    castle::math::random a(9u);
    castle::math::random b(9u);
    a.discard(0);
    EXPECT_EQ(a.next(), b.next());
}

} // namespace
