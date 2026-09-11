#include <gtest/gtest.h>

#include "castle/memory/construct.h"

namespace
{

int g_live = 0;

struct Counter
{
    int tag;

    Counter() CASTLE_NOEXCEPT : tag(0) { ++g_live; }
    explicit Counter(int t) CASTLE_NOEXCEPT : tag(t) { ++g_live; }
    Counter(int a, int b) CASTLE_NOEXCEPT : tag(a + b) { ++g_live; }
    Counter(CASTLE_CONST Counter& other) CASTLE_NOEXCEPT : tag(other.tag) { ++g_live; }
    ~Counter() { --g_live; }
};

TEST(ConstructTest, ConstructAtDefaultRunsConstructor)
{
    g_live = 0;
    alignas(Counter) unsigned char buf[sizeof(Counter)];

    Counter* p = castle::memory::construct_at<Counter>(&buf[0]);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(reinterpret_cast<void*>(p), static_cast<void*>(&buf[0]));
    EXPECT_EQ(p->tag, 0);
    EXPECT_EQ(g_live, 1);

    p->~Counter();
    EXPECT_EQ(g_live, 0);
}

TEST(ConstructTest, ConstructAtForwardsSingleArgument)
{
    g_live = 0;
    alignas(Counter) unsigned char buf[sizeof(Counter)];

    Counter* p = castle::memory::construct_at<Counter>(&buf[0], 5);
    EXPECT_EQ(p->tag, 5);
    EXPECT_EQ(g_live, 1);

    p->~Counter();
    EXPECT_EQ(g_live, 0);
}

TEST(ConstructTest, ConstructAtForwardsMultipleArguments)
{
    g_live = 0;
    alignas(Counter) unsigned char buf[sizeof(Counter)];

    Counter* p = castle::memory::construct_at<Counter>(&buf[0], 3, 4);
    EXPECT_EQ(p->tag, 7);
    EXPECT_EQ(g_live, 1);

    p->~Counter();
    EXPECT_EQ(g_live, 0);
}

TEST(ConstructTest, ConstructAtWorksForScalarTypes)
{
    alignas(int) unsigned char buf[sizeof(int)];
    int* p = castle::memory::construct_at<int>(&buf[0], 123);
    ASSERT_NE(p, nullptr);
    EXPECT_EQ(*p, 123);
}

TEST(ConstructTest, ConstructAtCopyConstructs)
{
    g_live = 0;
    Counter source(9);
    EXPECT_EQ(g_live, 1);

    alignas(Counter) unsigned char buf[sizeof(Counter)];
    Counter* p = castle::memory::construct_at<Counter>(&buf[0], source);
    EXPECT_EQ(p->tag, 9);
    EXPECT_EQ(g_live, 2);

    p->~Counter();
    EXPECT_EQ(g_live, 1);
}

} // namespace
