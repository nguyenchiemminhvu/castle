#include <gtest/gtest.h>
#include "castle/container/hash_table.hpp"

namespace
{
struct ConstantHash
{
    size_t operator()(int) const noexcept
    {
        return 0U;
    }
};

TEST(HashTableTest, BasicLookupAssignmentEmplaceAndIterators)
{
    using H=castle::container::hash_table<int,int,4,ConstantHash>;
    H h;
    static_assert(H::capacity()==4U,"capacity");
    EXPECT_TRUE(h.empty()); EXPECT_TRUE(h.begin()==h.end());
    EXPECT_EQ(h.insert(1,10),castle::status::ok);
    EXPECT_EQ(h.insert(1,11),castle::status::already_exists);
    EXPECT_EQ(h.emplace(2,20),castle::status::ok);
    EXPECT_EQ(h.emplace(2,30),castle::status::already_exists);
    EXPECT_EQ(h.try_emplace(3,30),castle::status::ok);
    EXPECT_TRUE(h.contains(2));
    EXPECT_FALSE(h.contains(9));
    EXPECT_EQ(*h.get(2),20);
    EXPECT_EQ(h.get(9),nullptr); auto it=h.find(1);
    EXPECT_NE(it,h.end());
    EXPECT_EQ(it->first,1);
    EXPECT_EQ(it->second,10);
    it->second=12;
    EXPECT_EQ(h.get(1)[0],12);
    EXPECT_EQ(h.insert_or_assign(1,15),castle::status::ok);
    EXPECT_EQ(*h.get(1),15);
    int v=16;
    EXPECT_EQ(h.insert_or_assign(1,std::move(v)),castle::status::ok);
    EXPECT_EQ(*h.get(1),16);
    EXPECT_EQ(h.insert_or_assign(4,40),castle::status::ok);
    EXPECT_TRUE(h.full());
    EXPECT_EQ(h.insert(5,50),castle::status::full);
    const H& ch=h;
    auto ci=ch.find(3);
    EXPECT_EQ(ci->second,30);
    size_t count=0;
    for(auto p=ch.cbegin();p!=ch.cend();++p)
    {
        ++count;
    }
    EXPECT_EQ(count,4U);
}

TEST(HashTableTest, TombstonesProbeAndClear)
{
    using H=castle::container::hash_table<int,int,3,ConstantHash>;
    H h;
    h.insert(1,1);
    h.insert(2,2);
    h.insert(3,3);
    EXPECT_EQ(h.erase(2),castle::status::ok);
    EXPECT_EQ(h.erase(2),castle::status::not_found);
    EXPECT_FALSE(h.contains(2));
    EXPECT_TRUE(h.contains(3));
    EXPECT_EQ(h.insert(4,4),castle::status::ok);
    EXPECT_EQ(*h.get(4),4);
    EXPECT_EQ(h.erase(9),castle::status::not_found);
    h.clear();
    EXPECT_TRUE(h.empty());
    EXPECT_EQ(h.available(),3U);
    EXPECT_EQ(h.erase(1),castle::status::not_found);
}

TEST(HashTableTest, InitializerListConstructor)
{
    using H=castle::container::hash_table<int,int,4,ConstantHash>;
    H h{{1,10},{2,20},{1,99}};
    EXPECT_EQ(h.size(),2U);
    EXPECT_EQ(*h.get(1),10);
    EXPECT_EQ(*h.get(2),20);

    H empty{};
    EXPECT_TRUE(empty.empty());
}

TEST(HashTableTest, InitializerListConstructorStopsWhenFull)
{
    using table_type = castle::container::hash_table<int, int, 3>;

    table_type table{
        {1, 11},
        {2, 22},
        {3, 33},
        {4, 44},
        {5, 55}
    };

    EXPECT_EQ(table.size(), 3U);
    EXPECT_TRUE(table.full());

    EXPECT_TRUE(table.contains(1));
    EXPECT_TRUE(table.contains(2));
    EXPECT_TRUE(table.contains(3));

    EXPECT_FALSE(table.contains(4));
    EXPECT_FALSE(table.contains(5));

    auto it1 = table.find(1);
    EXPECT_NE(it1, table.end());
    EXPECT_EQ(it1->second, 11);

    auto it4 = table.find(4);
    EXPECT_EQ(it4, table.end());
}

TEST(HashTableTest, DefaultConstructedConstIteratorIncrement)
{
    using table_type = castle::container::hash_table<int, int, 4>;
    typename table_type::const_iterator it;
    ++it; // covers: if (table_ != nullptr) == false
}

TEST(HashTableTest, InsertReusesFirstDeletedWhenEmptySlotAppearsLater)
{
    using H = castle::container::hash_table<int, int, 5, ConstantHash>;

    H h;

    ASSERT_EQ(h.insert(1, 10), castle::status::ok);
    ASSERT_EQ(h.insert(2, 20), castle::status::ok);

    // Layout (all hash to bucket 0):
    // [1][2][empty][empty][empty]

    ASSERT_EQ(h.erase(1), castle::status::ok);

    // Layout:
    // [deleted][2][empty][empty][empty]

    // insert_value():
    // step0 -> deleted => first_deleted = 0
    // step1 -> occupied
    // step2 -> empty
    //
    // Must take:
    // target = first_deleted
    // instead of target = index
    ASSERT_EQ(h.insert(3, 30), castle::status::ok);

    EXPECT_TRUE(h.contains(2));
    EXPECT_TRUE(h.contains(3));
    EXPECT_EQ(*h.get(3), 30);

    EXPECT_EQ(h.size(), 2U);
    EXPECT_EQ(h.available(), 3U);
}

TEST(HashTableTest, IteratorIncrementOnEndIterator)
{
    using H = castle::container::hash_table<int, int, 4>;

    H h;
    auto it = h.end();

    ++it; // advance_index(): index == N -> early return

    EXPECT_EQ(it, h.end());
}

TEST(HashTableTest, IteratorSkipsDeletedAndEmptySlots)
{
    using H = castle::container::hash_table<int, int, 5, ConstantHash>;

    H h;

    ASSERT_EQ(h.insert(1, 10), castle::status::ok);
    ASSERT_EQ(h.insert(2, 20), castle::status::ok);
    ASSERT_EQ(h.insert(3, 30), castle::status::ok);

    ASSERT_EQ(h.erase(2), castle::status::ok);

    // States become:
    // [occupied(1)][deleted(2)][occupied(3)][empty][empty]

    auto it = h.begin();
    ASSERT_NE(it, h.end());
    EXPECT_EQ(it->first, 1);

    ++it; // advance_index() must skip deleted slot(s)

    ASSERT_NE(it, h.end());
    EXPECT_EQ(it->first, 3);

    ++it; // advance_index() must skip trailing empty slot(s) to end

    EXPECT_EQ(it, h.end());
}

TEST(HashTableTest, DefaultConstructedIteratorIncrement)
{
    using H = castle::container::hash_table<int, int, 4>;
    typename H::iterator it;
    ++it; // covers: if (table_ != nullptr) == false
}

}
