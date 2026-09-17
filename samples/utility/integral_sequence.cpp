#include "sample_support.hpp"

#include "castle/utility/integral_sequence.hpp"

using manual_sequence = castle::sequence::index_sequence<0U, 1U, 2U>;
using merged_sequence = castle::sequence::merge_and_renumber<
    castle::sequence::index_sequence<0U, 1U>,
    castle::sequence::index_sequence<0U, 1U, 2U>>::type;
using built_sequence = castle::make_index_sequence<4U>::type;
using built_sequence_alias = castle::make_index_sequence_t<5U>;
using forwarded_sequence = castle::sequence::index_sequence_for<char, short, int>;

static_assert(manual_sequence::size() == 3U, "manual sequence size");
static_assert(merged_sequence::size() == 5U, "merged sequence size");
static_assert(built_sequence::size() == 4U, "built sequence size");
static_assert(built_sequence_alias::size() == 5U, "built sequence alias size");
static_assert(forwarded_sequence::size() == 3U, "forwarded sequence size");
static_assert(castle::sequence::sequence_size<forwarded_sequence>::value == 3U, "sequence_size");
static_assert(castle::sequence::sequence_size_t<forwarded_sequence>::value == 3U, "sequence_size_t");
static_assert(castle::sequence::is_index_sequence<forwarded_sequence>::value, "sequence trait");
static_assert(castle::sequence::is_index_sequence_v<forwarded_sequence>, "sequence variable trait");
static_assert(castle::is_index_sequence<castle::index_sequence<0U, 1U>>, "top-level sequence variable");

int main()
{
    CASTLE_SAMPLE_CHECK(built_sequence::size() == 4U);
    return 0;
}
