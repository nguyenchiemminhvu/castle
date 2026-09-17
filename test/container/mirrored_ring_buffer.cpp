#include <gtest/gtest.h>

#include "castle/container/mirrored_ring_buffer.hpp"
#include "castle/error/status.hpp"
#include "castle/iterator/traits.hpp"

namespace
{

template <typename Buffer>
void expect_contiguous_mirror(const Buffer& buffer)
{
    auto first = buffer.begin();
    for (castle::size_type i = 0U; i < buffer.size(); ++i)
    {
        EXPECT_EQ(first[i], first[i + buffer.capacity()]);
    }
}

TEST(MirroredRingBufferTest, BasicPushPopAndRandomAccess)
{
    using R = castle::container::mirrored_ring_buffer<int, 4>;
    static_assert(R::static_capacity == 4U, "capacity");
    static_assert(noexcept(R{}), "constructor must be noexcept");
    static_assert(castle::meta::is_same<
                      typename castle::iterator_traits<typename R::iterator>::iterator_category,
                      castle::random_access_iterator_tag>::value,
                  "raw pointers must remain random access iterators");

    R r;
    EXPECT_EQ(R::capacity(), 4U);
    EXPECT_EQ(R::max_size(), 4U);
    EXPECT_TRUE(r.empty());
    EXPECT_FALSE(r.full());
    EXPECT_EQ(r.size(), 0U);
    EXPECT_EQ(r.available(), 4U);
    EXPECT_EQ(r.begin(), r.end());
    EXPECT_EQ(r.data(), r.begin());
    EXPECT_EQ(r.pop(), castle::status::empty);

    int first_value = 1;
    EXPECT_EQ(r.push(first_value), castle::status::ok);
    int x = 2;
    EXPECT_EQ(r.push(castle::move(x)), castle::status::ok);
    EXPECT_EQ(r.push(3), castle::status::ok);
    EXPECT_EQ(r.size(), 3U);
    EXPECT_FALSE(r.empty());
    EXPECT_FALSE(r.full());
    EXPECT_EQ(r.front(), 1);
    EXPECT_EQ(r.back(), 3);
    EXPECT_EQ(r[0U], 1);
    EXPECT_EQ(r[1U], 2);
    EXPECT_EQ(r[2U], 3);
    int peeked = 0;
    EXPECT_EQ(r.peek(1U, peeked), castle::status::ok);
    EXPECT_EQ(peeked, 2);
    EXPECT_EQ(r.peek(3U, peeked), castle::status::out_of_range);
    expect_contiguous_mirror(r);

    int sum = 0;
    for (auto it = r.begin(); it != r.end(); ++it)
    {
        sum += *it;
    }
    EXPECT_EQ(sum, 6);

    const R& cr = r;
    EXPECT_EQ(cr.cbegin(), cr.begin());
    EXPECT_EQ(cr.cend(), cr.end());
    EXPECT_EQ(cr[1U], 2);
    EXPECT_EQ(cr.data(), cr.begin());

    int out = 0;
    EXPECT_EQ(r.pop(out), castle::status::ok);
    EXPECT_EQ(out, 1);
    EXPECT_EQ(r.pop(), castle::status::ok);
    EXPECT_EQ(r.front(), 3);
    EXPECT_EQ(r.back(), 3);
    EXPECT_EQ(r.size(), 1U);
    expect_contiguous_mirror(r);

    EXPECT_EQ(r.pop(out), castle::status::ok);
    EXPECT_EQ(out, 3);
    EXPECT_TRUE(r.empty());
    EXPECT_FALSE(r.full());
    EXPECT_EQ(r.pop(out), castle::status::empty);
}

TEST(MirroredRingBufferTest, ReservationAndCommitBranches)
{
    using R = castle::container::mirrored_ring_buffer<int, 4>;
    R r;

    auto write = r.write_reserve();
    EXPECT_EQ(write.size(), 4U);
    for (castle::size_type i = 0U; i < write.size(); ++i)
    {
        write[i] = static_cast<int>(i + 1U);
    }
    EXPECT_EQ(r.write_commit(write), castle::status::ok);
    EXPECT_TRUE(r.full());
    expect_contiguous_mirror(r);

    auto no_write = r.write_reserve(2U);
    EXPECT_TRUE(no_write.empty());
    EXPECT_EQ(r.write_commit(no_write), castle::status::ok);

    auto optimal_full = r.write_reserve_optimal();
    EXPECT_TRUE(optimal_full.empty());
    EXPECT_TRUE(r.write_reserve_optimal(1U).empty());

    auto read = r.read_reserve(2U);
    EXPECT_EQ(read.size(), 2U);
    EXPECT_EQ(read[0], 1);
    EXPECT_EQ(read[1], 2);
    read[0] = 10;
    EXPECT_EQ(read[0], 10);
    EXPECT_EQ(r.read_commit(read), castle::status::ok);
    EXPECT_EQ(r.front(), 3);
    EXPECT_EQ(r.size(), 2U);
    expect_contiguous_mirror(r);

    auto invalid_write_pointer = castle::container::array_view<int>(r.data() + 1U, 1U);
    EXPECT_EQ(r.write_commit(invalid_write_pointer), castle::status::invalid_argument);

    int external = 7;
    auto invalid_write_object = castle::container::array_view<int>(&external, 1U);
    EXPECT_EQ(r.write_commit(invalid_write_object), castle::status::invalid_argument);

    auto valid_write = r.write_reserve(4U);
    EXPECT_EQ(valid_write.size(), 2U);
    valid_write[0] = 20;
    valid_write[1] = 21;

    auto oversized_write = castle::container::array_view<int>(valid_write.data(), 5U);
    EXPECT_EQ(r.write_commit(oversized_write), castle::status::full);
    EXPECT_EQ(r.size(), 2U);
    EXPECT_EQ(r.write_commit(valid_write), castle::status::ok);
    EXPECT_TRUE(r.full());

    auto full_read = r.read_reserve(99U);
    EXPECT_EQ(full_read.size(), 4U);
    EXPECT_EQ(r.read_commit(full_read), castle::status::ok);
    EXPECT_TRUE(r.empty());

    auto optimal_empty = r.write_reserve_optimal(2U);
    EXPECT_EQ(optimal_empty.size(), 4U);
    for (castle::size_type i = 0U; i < optimal_empty.size(); ++i)
    {
        optimal_empty[i] = static_cast<int>(30U + i);
    }
    EXPECT_EQ(r.write_commit(optimal_empty), castle::status::ok);
    EXPECT_TRUE(r.full());
    auto drain_optimal = r.read_reserve();
    EXPECT_EQ(r.read_commit(drain_optimal), castle::status::ok);
    EXPECT_TRUE(r.empty());

    auto one_write = r.write_reserve(1U);
    one_write[0] = 40;
    EXPECT_EQ(r.write_commit(one_write), castle::status::ok);
    auto one_read = r.read_reserve(1U);
    ASSERT_EQ(one_read.size(), 1U);
    auto oversized_read = castle::container::array_view<int>(one_read.data(), 2U);
    EXPECT_EQ(r.read_commit(oversized_read), castle::status::empty);
    EXPECT_EQ(r.read_commit(one_read), castle::status::ok);
    EXPECT_TRUE(r.empty());

    auto invalid_read_pointer = castle::container::array_view<int>(r.data() + 1U, 1U);
    EXPECT_EQ(r.read_commit(invalid_read_pointer), castle::status::invalid_argument);

    auto no_read = r.read_reserve(2U);
    EXPECT_TRUE(no_read.empty());
    EXPECT_EQ(r.read_commit(no_read), castle::status::ok);

    int external_read = 0;
    auto invalid_read_object = castle::container::array_view<int>(&external_read, 1U);
    EXPECT_EQ(r.read_commit(invalid_read_object), castle::status::invalid_argument);
}

TEST(MirroredRingBufferTest, WrapKeepsTheSequenceContiguousAndMirrored)
{
    using R = castle::container::mirrored_ring_buffer<int, 4>;
    R r;

    EXPECT_EQ(r.push(1), castle::status::ok);
    EXPECT_EQ(r.push(2), castle::status::ok);
    EXPECT_EQ(r.push(3), castle::status::ok);
    int out = 0;
    EXPECT_EQ(r.pop(out), castle::status::ok);
    EXPECT_EQ(out, 1);
    EXPECT_EQ(r.push(4), castle::status::ok);
    EXPECT_EQ(r.push(5), castle::status::ok);
    EXPECT_TRUE(r.full());
    int lvalue_full = 6;
    EXPECT_EQ(r.push(lvalue_full), castle::status::full);
    EXPECT_EQ(r.push(7), castle::status::full);

    // head is physically at slot 1, so the logical sequence spans the
    // physical wrap while remaining contiguous through the mirror.
    ASSERT_EQ(r.size(), 4U);
    EXPECT_EQ(r.begin()[0U], 2);
    EXPECT_EQ(r.begin()[1U], 3);
    EXPECT_EQ(r.begin()[2U], 4);
    EXPECT_EQ(r.begin()[3U], 5);

    auto read = r.read_reserve(4U);
    ASSERT_EQ(read.size(), 4U);
    EXPECT_EQ(read.data(), r.begin());
    EXPECT_EQ(read[0], 2);
    EXPECT_EQ(read[3], 5);
    EXPECT_EQ(r.read_commit(read), castle::status::ok);
    EXPECT_TRUE(r.empty());
}

TEST(MirroredRingBufferTest, ReadMutationOfWrappedReservationIsOverwrittenByNextWrite)
{
    using R = castle::container::mirrored_ring_buffer<int, 4>;
    R r;

    EXPECT_EQ(r.push(1), castle::status::ok);
    EXPECT_EQ(r.push(2), castle::status::ok);
    EXPECT_EQ(r.push(3), castle::status::ok);
    int out = 0;
    EXPECT_EQ(r.pop(out), castle::status::ok);
    EXPECT_EQ(out, 1);
    EXPECT_EQ(r.push(4), castle::status::ok);
    EXPECT_EQ(r.push(5), castle::status::ok);
    EXPECT_TRUE(r.full());
    ASSERT_EQ(r.size(), 4U);

    // The wrapped reservation's last slot is physically the mirror copy that
    // backs logical position 4; mutate it in place, as read_reserve() allows.
    auto read = r.read_reserve(4U);
    ASSERT_EQ(read.size(), 4U);
    read[3U] = 500;
    EXPECT_EQ(r.read_commit(read), castle::status::ok);
    EXPECT_TRUE(r.empty());

    // commit_read() no longer resyncs the primary copy of the freed slot with
    // the edit above; the next write to reuse that slot must still win.
    EXPECT_EQ(r.push(6), castle::status::ok);
    EXPECT_EQ(r.push(7), castle::status::ok);
    EXPECT_EQ(r.push(8), castle::status::ok);
    EXPECT_EQ(r.push(9), castle::status::ok);
    ASSERT_EQ(r.size(), 4U);
    EXPECT_EQ(r.begin()[0U], 6);
    EXPECT_EQ(r.begin()[1U], 7);
    EXPECT_EQ(r.begin()[2U], 8);
    EXPECT_EQ(r.begin()[3U], 9);
}

TEST(MirroredRingBufferTest, NonPowerOfTwoAndBulkPaths)
{
    using R = castle::container::mirrored_ring_buffer<int, 3>;
    R r;

    int src[] = {6, 7, 8, 9};
    EXPECT_EQ(r.push_bulk(nullptr, 3U), 0U);
    EXPECT_EQ(r.push_bulk(src, 4U), 3U);
    EXPECT_TRUE(r.full());
    expect_contiguous_mirror(r);
    EXPECT_EQ(r.front(), 6);
    int peeked = 0;
    EXPECT_EQ(r.peek(1U, peeked), castle::status::ok);
    EXPECT_EQ(peeked, 7);
    EXPECT_EQ(r.peek(3U, peeked), castle::status::out_of_range);
    int full_lvalue = 10;
    EXPECT_EQ(r.push(full_lvalue), castle::status::full);
    int full_rvalue = 11;
    EXPECT_EQ(r.push(castle::move(full_rvalue)), castle::status::full);

    int dst[5] = {};
    EXPECT_EQ(r.pop_bulk(nullptr, 5U), 0U);
    EXPECT_EQ(r.pop_bulk(dst, 5U), 3U);
    EXPECT_EQ(dst[0], 6);
    EXPECT_EQ(dst[2], 8);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.pop_bulk(dst, 2U), 0U);

    int out = 0;
    EXPECT_EQ(r.pop(), castle::status::empty);
    int lvalue = 10;
    EXPECT_EQ(r.push(lvalue), castle::status::ok);
    EXPECT_EQ(r.pop(), castle::status::ok);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.pop(out), castle::status::empty);

    auto empty_write = r.write_reserve(0U);
    EXPECT_TRUE(empty_write.empty());
    EXPECT_EQ(r.write_commit(empty_write), castle::status::ok);
    auto empty_read = r.read_reserve(0U);
    EXPECT_TRUE(empty_read.empty());
    EXPECT_EQ(r.read_commit(empty_read), castle::status::ok);

    EXPECT_TRUE(r.write_reserve_optimal(1U).size() == 3U);
    EXPECT_TRUE(r.write_reserve_optimal(4U).empty());

    int external = 99;
    auto invalid_write = castle::container::array_view<int>(&external, 1U);
    EXPECT_EQ(r.write_commit(invalid_write), castle::status::invalid_argument);
    auto valid_write = r.write_reserve(1U);
    auto oversized_write = castle::container::array_view<int>(valid_write.data(), 4U);
    EXPECT_EQ(r.write_commit(oversized_write), castle::status::full);
    valid_write[0] = 11;
    EXPECT_EQ(r.write_commit(valid_write), castle::status::ok);

    auto valid_read = r.read_reserve(1U);
    auto invalid_read = castle::container::array_view<int>(&external, 1U);
    EXPECT_EQ(r.read_commit(invalid_read), castle::status::invalid_argument);
    auto oversized_read = castle::container::array_view<int>(valid_read.data(), 2U);
    EXPECT_EQ(r.read_commit(oversized_read), castle::status::empty);
    EXPECT_EQ(r.read_commit(valid_read), castle::status::ok);

    r.push(11);
    r.push(12);
    EXPECT_EQ(r.size(), 2U);
    r.clear();
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.available(), 3U);
}

TEST(MirroredRingBufferTest, ReservationSizesAreClampedAndMirrorAcrossBoundary)
{
    using R = castle::container::mirrored_ring_buffer<int, 3>;
    R r;

    EXPECT_EQ(r.push(1), castle::status::ok);
    EXPECT_EQ(r.push(2), castle::status::ok);
    int out = 0;
    EXPECT_EQ(r.pop(out), castle::status::ok);
    EXPECT_EQ(out, 1);

    // write_index is now at logical slot 2. A two-element reservation crosses
    // the first physical half and exercises both mirror_range branches.
    auto write = r.write_reserve(99U);
    ASSERT_EQ(write.size(), 2U);
    write[0] = 3;
    write[1] = 4;
    EXPECT_EQ(r.write_commit(write), castle::status::ok);
    EXPECT_TRUE(r.full());
    ASSERT_EQ(r.size(), 3U);
    EXPECT_EQ(r.begin()[0U], 2);
    EXPECT_EQ(r.begin()[1U], 3);
    EXPECT_EQ(r.begin()[2U], 4);
    auto read = r.read_reserve(99U);
    EXPECT_EQ(read.size(), 3U);
    read[0] = 20;
    read[1] = 30;
    read[2] = 40;
    EXPECT_EQ(r.read_commit(read), castle::status::ok);
    EXPECT_TRUE(r.empty());
}

TEST(MirroredRingBufferTest, EmptyCommitRejectsStalePointer)
{
    using R = castle::container::mirrored_ring_buffer<int, 4>;
    R r;

    // An empty reservation must still match the current index; a stale or
    // foreign pointer paired with size 0 is not a silent no-op success.
    int external = 0;
    auto stale_empty_write = castle::container::array_view<int>(&external, 0U);
    EXPECT_EQ(r.write_commit(stale_empty_write), castle::status::invalid_argument);

    auto stale_empty_read = castle::container::array_view<int>(&external, 0U);
    EXPECT_EQ(r.read_commit(stale_empty_read), castle::status::invalid_argument);
}

TEST(MirroredRingBufferTest, BulkPathsMirrorAcrossPhysicalBoundary)
{
    using R = castle::container::mirrored_ring_buffer<int, 3>;
    R r;

    EXPECT_EQ(r.push(1), castle::status::ok);
    EXPECT_EQ(r.push(2), castle::status::ok);
    int out = 0;
    EXPECT_EQ(r.pop(out), castle::status::ok);
    EXPECT_EQ(out, 1);

    // write_index is now at logical slot 2; a 2-element bulk write crosses
    // the physical boundary and exercises both mirror_range copy loops.
    // (expect_contiguous_mirror() is not used here: it assumes an offset of
    // 0 and would index past the storage array once the window itself wraps
    // while full, so the wrapped values are asserted directly instead.)
    int src[] = {3, 4};
    EXPECT_EQ(r.push_bulk(src, 2U), 2U);
    EXPECT_TRUE(r.full());
    ASSERT_EQ(r.size(), 3U);
    EXPECT_EQ(r.begin()[0U], 2);
    EXPECT_EQ(r.begin()[1U], 3);
    EXPECT_EQ(r.begin()[2U], 4);

    int dst[3] = {};
    EXPECT_EQ(r.pop_bulk(dst, 3U), 3U);
    EXPECT_EQ(dst[0], 2);
    EXPECT_EQ(dst[1], 3);
    EXPECT_EQ(dst[2], 4);
    EXPECT_TRUE(r.empty());
}
} // namespace
