#include <gtest/gtest.h>

#include "castle/utility/integral_sequence.h"
#include "castle/core/traits.h"

namespace
{

template <castle::size_type... Indices>
constexpr castle::size_type index_sum(castle::sequence::index_sequence<Indices...>)
{
    return (castle::size_type(0) + ... + Indices);
}

TEST(IntegralSequenceTest, EmptySequenceSize)
{
    using seq = castle::make_index_sequence_t<0>;
    static_assert(castle::meta::is_same<seq, castle::sequence::index_sequence<>>::value,
                  "make_index_sequence_t<0> must be empty");
    EXPECT_EQ(seq::size(), 0U);
}

TEST(IntegralSequenceTest, SingleElementSequence)
{
    using seq = castle::make_index_sequence_t<1>;
    static_assert(castle::meta::is_same<seq, castle::sequence::index_sequence<0>>::value,
                  "make_index_sequence_t<1> must be <0>");
    EXPECT_EQ(seq::size(), 1U);
}

TEST(IntegralSequenceTest, GeneratesConsecutiveIndices)
{
    using seq = castle::make_index_sequence_t<4>;
    static_assert(castle::meta::is_same<seq, castle::sequence::index_sequence<0, 1, 2, 3>>::value,
                  "make_index_sequence_t<4> must be <0,1,2,3>");
    EXPECT_EQ(seq::size(), 4U);
    EXPECT_EQ(index_sum(seq{}), 6U);
}

TEST(IntegralSequenceTest, LargerSequence)
{
    using seq = castle::make_index_sequence_t<10>;
    EXPECT_EQ(seq::size(), 10U);
    EXPECT_EQ(index_sum(seq{}), 45U);
}

TEST(IntegralSequenceTest, IndexSequenceForCountsTypes)
{
    using seq = castle::sequence::index_sequence_for<int, char, double>;
    static_assert(castle::meta::is_same<seq, castle::sequence::index_sequence<0, 1, 2>>::value,
                  "index_sequence_for must count parameter pack");
    EXPECT_EQ(seq::size(), 3U);
}

TEST(IntegralSequenceTest, ValueTypeIsSizeType)
{
    using seq = castle::sequence::index_sequence<0, 1>;
    static_assert(castle::meta::is_same<seq::value_type, castle::size_type>::value,
                  "index_sequence::value_type must be size_type");
}

TEST(IntegralSequenceTest, IsIndexSequenceTrait)
{
    static_assert(castle::sequence::is_index_sequence<castle::sequence::index_sequence<0, 1>>::value,
                  "index_sequence must satisfy is_index_sequence");
    static_assert(!castle::sequence::is_index_sequence<int>::value,
                  "int must not satisfy is_index_sequence");
}

TEST(IntegralSequenceTest, IsIndexSequenceVariableAlias)
{
    static_assert(castle::is_index_sequence<castle::index_sequence<0, 1, 2>>,
                  "castle::is_index_sequence alias must report true");
    static_assert(!castle::is_index_sequence<double>,
                  "castle::is_index_sequence alias must report false for non-sequence");
}

TEST(IntegralSequenceTest, CastleAliasMatchesSequenceType)
{
    static_assert(castle::meta::is_same<castle::index_sequence<0, 1>,
                                        castle::sequence::index_sequence<0, 1>>::value,
                  "castle::index_sequence must alias sequence::index_sequence");
}

} // namespace
