#include <gtest/gtest.h>

#include "castle/memory/soo_buffer.h"

#include <utility>

namespace
{

int g_live = 0;

struct Counter
{
    int tag;

    Counter() CASTLE_NOEXCEPT : tag(0) { ++g_live; }
    explicit Counter(int t) CASTLE_NOEXCEPT : tag(t) { ++g_live; }
    Counter(Counter&& other) CASTLE_NOEXCEPT : tag(other.tag) { ++g_live; }
    ~Counter() { --g_live; }
};

TEST(SooBufferTest, StoresAndExposesValue)
{
    castle::memory::soo_buffer<int> buffer{42};
    ASSERT_NE(buffer.get(), nullptr);
    EXPECT_EQ(*buffer.get(), 42);
}

TEST(SooBufferTest, GetAllowsMutation)
{
    castle::memory::soo_buffer<int> buffer{1};
    *buffer.get() = 999;
    EXPECT_EQ(*buffer.get(), 999);
}

TEST(SooBufferTest, ConstGetReturnsConstPointer)
{
    castle::memory::soo_buffer<int> buffer{7};
    CASTLE_CONST auto& cref = buffer;
    ASSERT_NE(cref.get(), nullptr);
    EXPECT_EQ(*cref.get(), 7);
}

TEST(SooBufferTest, MoveConstructionPreservesValue)
{
    castle::memory::soo_buffer<int> a{123};
    castle::memory::soo_buffer<int> b{std::move(a)};
    EXPECT_EQ(*b.get(), 123);
}

TEST(SooBufferTest, MoveAssignmentPreservesValue)
{
    castle::memory::soo_buffer<int> a{321};
    castle::memory::soo_buffer<int> b{0};
    b = std::move(a);
    EXPECT_EQ(*b.get(), 321);
}

TEST(SooBufferTest, RunsDestructorOnScopeExit)
{
    g_live = 0;
    {
        castle::memory::soo_buffer<Counter> buffer{Counter{}};
        EXPECT_EQ(g_live, 1);
        EXPECT_EQ(buffer.get()->tag, 0);
    }
    EXPECT_EQ(g_live, 0);
}

TEST(SooBufferTest, HonorsCustomStackSize)
{
    // StackSize larger than sizeof(T) still stores the object inline.
    castle::memory::soo_buffer<int, sizeof(int) * 4U> buffer{55};
    EXPECT_EQ(*buffer.get(), 55);
}

} // namespace
