#include <gtest/gtest.h>

#include "castle/iterator/operations.h"

#include <type_traits>

namespace
{

// Forward-only iterator (tagged input) wrapping a raw pointer. Provides just
// enough surface to drive the input_iterator_tag code paths.
template <typename T>
class forward_only_iterator
{
public:
    using difference_type = castle::difference_type;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = castle::input_iterator_tag;

    forward_only_iterator() : ptr_(nullptr) {}
    explicit forward_only_iterator(T* ptr) : ptr_(ptr) {}

    reference operator*() const { return *ptr_; }

    forward_only_iterator& operator++()
    {
        ++ptr_;
        return *this;
    }

    bool operator==(const forward_only_iterator& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const forward_only_iterator& other) const { return ptr_ != other.ptr_; }

private:
    T* ptr_;
};

// Bidirectional iterator wrapping a raw pointer.
template <typename T>
class bidirectional_iterator_adapter
{
public:
    using difference_type = castle::difference_type;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using iterator_category = castle::bidirectional_iterator_tag;

    bidirectional_iterator_adapter() : ptr_(nullptr) {}
    explicit bidirectional_iterator_adapter(T* ptr) : ptr_(ptr) {}

    reference operator*() const { return *ptr_; }

    bidirectional_iterator_adapter& operator++()
    {
        ++ptr_;
        return *this;
    }

    bidirectional_iterator_adapter& operator--()
    {
        --ptr_;
        return *this;
    }

    bool operator==(const bidirectional_iterator_adapter& other) const { return ptr_ == other.ptr_; }
    bool operator!=(const bidirectional_iterator_adapter& other) const { return ptr_ != other.ptr_; }

private:
    T* ptr_;
};

TEST(IteratorOperationsTest, DistanceRandomAccessPointers)
{
    int data[5] = {0, 1, 2, 3, 4};
    EXPECT_EQ(castle::distance(data, data + 5), 5);
    EXPECT_EQ(castle::distance(data + 1, data + 4), 3);
    EXPECT_EQ(castle::distance(data, data), 0);
}

TEST(IteratorOperationsTest, DistanceInputIterator)
{
    int data[4] = {10, 20, 30, 40};
    forward_only_iterator<int> first(data);
    forward_only_iterator<int> last(data + 4);
    EXPECT_EQ(castle::distance(first, last), 4);
    EXPECT_EQ(castle::distance(first, first), 0);
}

TEST(IteratorOperationsTest, DistanceBidirectionalIterator)
{
    int data[3] = {1, 2, 3};
    bidirectional_iterator_adapter<int> first(data);
    bidirectional_iterator_adapter<int> last(data + 3);
    // bidirectional lacks its own overload -> resolves to input_iterator_tag.
    EXPECT_EQ(castle::distance(first, last), 3);
}

TEST(IteratorOperationsTest, AdvanceRandomAccessForwardAndBackward)
{
    int data[6] = {0, 1, 2, 3, 4, 5};
    int* it = data;
    castle::advance(it, 3);
    EXPECT_EQ(*it, 3);
    castle::advance(it, -2);
    EXPECT_EQ(*it, 1);
    castle::advance(it, 0);
    EXPECT_EQ(*it, 1);
}

TEST(IteratorOperationsTest, AdvanceInputIteratorForwardOnly)
{
    int data[4] = {5, 6, 7, 8};
    forward_only_iterator<int> it(data);
    castle::advance(it, 2);
    EXPECT_EQ(*it, 7);
    // Non-positive offsets are ignored for input iterators.
    castle::advance(it, -1);
    EXPECT_EQ(*it, 7);
    castle::advance(it, 0);
    EXPECT_EQ(*it, 7);
}

TEST(IteratorOperationsTest, AdvanceBidirectionalForwardAndBackward)
{
    int data[5] = {2, 4, 6, 8, 10};
    bidirectional_iterator_adapter<int> it(data);
    castle::advance(it, 4);
    EXPECT_EQ(*it, 10);
    castle::advance(it, -3);
    EXPECT_EQ(*it, 4);
}

TEST(IteratorOperationsTest, NextReturnsAdvancedCopyWithoutMutating)
{
    int data[5] = {0, 1, 2, 3, 4};
    int* it = data + 1;
    int* n = castle::next(it, 2);
    EXPECT_EQ(*n, 3);
    EXPECT_EQ(*it, 1); // original unchanged
    EXPECT_EQ(*castle::next(it), 2); // default offset 1
}

TEST(IteratorOperationsTest, PrevReturnsBackwardCopyWithoutMutating)
{
    int data[5] = {0, 1, 2, 3, 4};
    int* it = data + 4;
    int* p = castle::prev(it, 2);
    EXPECT_EQ(*p, 2);
    EXPECT_EQ(*it, 4); // original unchanged
    EXPECT_EQ(*castle::prev(it), 3); // default offset 1
}

TEST(IteratorOperationsTest, NextPrevBidirectional)
{
    int data[4] = {1, 2, 3, 4};
    bidirectional_iterator_adapter<int> it(data + 1);
    EXPECT_EQ(*castle::next(it, 2), 4);
    EXPECT_EQ(*castle::prev(it, 1), 1);
}

} // namespace
