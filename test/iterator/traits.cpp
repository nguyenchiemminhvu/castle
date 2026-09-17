#include <gtest/gtest.h>

#include "castle/iterator/traits.h"

#include <type_traits>

namespace
{

// Minimal user-defined iterator exposing the canonical member typedefs so the
// primary iterator_traits template can forward them.
struct custom_iterator
{
    using difference_type = long;
    using value_type = double;
    using pointer = double*;
    using reference = double&;
    using iterator_category = castle::bidirectional_iterator_tag;
};

TEST(IteratorTraitsTest, PointerSpecialization)
{
    using traits = castle::iterator_traits<int*>;
    static_assert(std::is_same<traits::value_type, int>::value, "value_type");
    static_assert(std::is_same<traits::pointer, int*>::value, "pointer");
    static_assert(std::is_same<traits::reference, int&>::value, "reference");
    static_assert(std::is_same<traits::difference_type,
                               castle::difference_type>::value,
                  "difference_type");
    static_assert(std::is_same<traits::iterator_category,
                               castle::random_access_iterator_tag>::value,
                  "category");
    SUCCEED();
}

TEST(IteratorTraitsTest, ConstPointerSpecialization)
{
    using traits = castle::iterator_traits<const int*>;
    static_assert(std::is_same<traits::value_type, int>::value, "value_type");
    static_assert(std::is_same<traits::pointer, const int*>::value, "pointer");
    static_assert(std::is_same<traits::reference, const int&>::value, "reference");
    static_assert(std::is_same<traits::difference_type,
                               castle::difference_type>::value,
                  "difference_type");
    static_assert(std::is_same<traits::iterator_category,
                               castle::random_access_iterator_tag>::value,
                  "category");
    SUCCEED();
}

TEST(IteratorTraitsTest, CustomIteratorForwardsMemberTypedefs)
{
    using traits = castle::iterator_traits<custom_iterator>;
    static_assert(std::is_same<traits::difference_type, long>::value, "difference_type");
    static_assert(std::is_same<traits::value_type, double>::value, "value_type");
    static_assert(std::is_same<traits::pointer, double*>::value, "pointer");
    static_assert(std::is_same<traits::reference, double&>::value, "reference");
    static_assert(std::is_same<traits::iterator_category,
                               castle::bidirectional_iterator_tag>::value,
                  "category");
    SUCCEED();
}

TEST(IteratorTraitsTest, DifferenceTypeIsPtrdiff)
{
    static_assert(std::is_same<castle::difference_type, std::ptrdiff_t>::value,
                  "difference_type is ptrdiff_t");
    SUCCEED();
}

} // namespace
