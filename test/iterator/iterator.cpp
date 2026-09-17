#include <gtest/gtest.h>

#include "castle/iterator/iterator.h"

#include <type_traits>

namespace
{

// The umbrella header should pull in traits, operations, reverse_iterator,
// circular_iterator and fixed_iterator together.
TEST(IteratorUmbrellaTest, TraitsAndOperationsAvailable)
{
    int data[5] = {0, 1, 2, 3, 4};
    EXPECT_EQ(castle::distance(data, data + 5), 5);
    int* it = data;
    castle::advance(it, 2);
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(*castle::next(it), 3);
    EXPECT_EQ(*castle::prev(it), 1);

    static_assert(std::is_same<
                      castle::iterator_traits<int*>::iterator_category,
                      castle::random_access_iterator_tag>::value,
                  "traits available");
    SUCCEED();
}

TEST(IteratorUmbrellaTest, ReverseIteratorAvailable)
{
    int data[3] = {1, 2, 3};
    castle::reverse_iterator<int*> it(data + 3);
    EXPECT_EQ(*it, 3);
    ++it;
    EXPECT_EQ(*it, 2);
}

TEST(IteratorUmbrellaTest, CircularIteratorAvailable)
{
    int data[2] = {8, 9};
    castle::circular_iterator<int*> it(data, data + 2, data + 1);
    EXPECT_EQ(*it, 9);
    ++it; // wraps
    EXPECT_EQ(*it, 8);
}

TEST(IteratorUmbrellaTest, FixedIteratorAvailable)
{
    int data[2] = {4, 5};
    castle::fixed_iterator<int*> it(data, data, data + 2);
    EXPECT_TRUE(it.valid());
    ++it;
    EXPECT_EQ(*it, 5);
}

TEST(IteratorUmbrellaTest, ReverseOverCircularCompositesTypedefs)
{
    // Compose reverse_iterator on a raw pointer and verify traversal to ensure
    // the whole subsystem interoperates when reached through the umbrella.
    int data[4] = {1, 2, 3, 4};
    castle::reverse_iterator<int*> first(data + 4);
    castle::reverse_iterator<int*> last(data);
    int expected = 4;
    for (auto it = first; it != last; ++it)
    {
        EXPECT_EQ(*it, expected--);
    }
    EXPECT_EQ(expected, 0);
}

} // namespace
