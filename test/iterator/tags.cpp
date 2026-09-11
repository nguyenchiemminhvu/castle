#include <gtest/gtest.h>

#include "castle/iterator/tags.h"

#include <type_traits>

namespace
{

TEST(IteratorTagsTest, TagsAreDistinctEmptyTypes)
{
    static_assert(std::is_empty<castle::input_iterator_tag>::value, "empty");
    static_assert(std::is_empty<castle::output_iterator_tag>::value, "empty");
    static_assert(std::is_empty<castle::forward_iterator_tag>::value, "empty");
    static_assert(std::is_empty<castle::bidirectional_iterator_tag>::value, "empty");
    static_assert(std::is_empty<castle::random_access_iterator_tag>::value, "empty");

    static_assert(!std::is_same<castle::input_iterator_tag,
                                castle::output_iterator_tag>::value,
                  "distinct");
    static_assert(!std::is_same<castle::forward_iterator_tag,
                                castle::bidirectional_iterator_tag>::value,
                  "distinct");
    SUCCEED();
}

TEST(IteratorTagsTest, HierarchyThroughInheritance)
{
    static_assert(std::is_base_of<castle::input_iterator_tag,
                                  castle::forward_iterator_tag>::value,
                  "forward is-a input");
    static_assert(std::is_base_of<castle::forward_iterator_tag,
                                  castle::bidirectional_iterator_tag>::value,
                  "bidirectional is-a forward");
    static_assert(std::is_base_of<castle::bidirectional_iterator_tag,
                                  castle::random_access_iterator_tag>::value,
                  "random_access is-a bidirectional");

    // Transitivity: random access derives from input through the chain.
    static_assert(std::is_base_of<castle::input_iterator_tag,
                                  castle::random_access_iterator_tag>::value,
                  "random_access is-a input");
    static_assert(std::is_base_of<castle::input_iterator_tag,
                                  castle::bidirectional_iterator_tag>::value,
                  "bidirectional is-a input");
    SUCCEED();
}

TEST(IteratorTagsTest, OutputTagIsIndependent)
{
    static_assert(!std::is_base_of<castle::input_iterator_tag,
                                   castle::output_iterator_tag>::value,
                  "output not derived from input");
    static_assert(!std::is_base_of<castle::output_iterator_tag,
                                   castle::forward_iterator_tag>::value,
                  "forward not derived from output");
    SUCCEED();
}

TEST(IteratorTagsTest, DerivedTagConvertsToBaseTag)
{
    // A more-capable tag can be used where a less-capable one is expected.
    castle::random_access_iterator_tag random;
    castle::bidirectional_iterator_tag bidirectional = random;
    castle::forward_iterator_tag forward = bidirectional;
    castle::input_iterator_tag input = forward;
    (void)input;
    SUCCEED();
}

} // namespace
