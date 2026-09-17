#include <gtest/gtest.h>

#include "castle/memory/destroy.h"
#include "castle/memory/new.h"

namespace
{

int g_live = 0;

struct Counter
{
    Counter() CASTLE_NOEXCEPT { ++g_live; }
    ~Counter() { --g_live; }
};

TEST(DestroyTest, DestroyAtRunsDestructor)
{
    g_live = 0;
    alignas(Counter) unsigned char buf[sizeof(Counter)];
    Counter* p = ::new (static_cast<void*>(&buf[0])) Counter();
    EXPECT_EQ(g_live, 1);

    castle::memory::destroy_at(p);
    EXPECT_EQ(g_live, 0);
}

TEST(DestroyTest, DestroyAtIsSafeForTrivialTypes)
{
    alignas(int) unsigned char buf[sizeof(int)];
    int* p = ::new (static_cast<void*>(&buf[0])) int(77);
    // Trivial destructor: no-op, but must compile and leave value readable.
    castle::memory::destroy_at(p);
    EXPECT_EQ(*p, 77);
}

TEST(DestroyTest, DestroyNRunsEachDestructor)
{
    g_live = 0;
    constexpr size_t kCount = 5;
    alignas(Counter) unsigned char buf[sizeof(Counter) * kCount];
    Counter* base = reinterpret_cast<Counter*>(&buf[0]);

    for (size_t i = 0; i < kCount; ++i)
    {
        ::new (static_cast<void*>(base + i)) Counter();
    }
    EXPECT_EQ(g_live, static_cast<int>(kCount));

    castle::memory::destroy_n(base, kCount);
    EXPECT_EQ(g_live, 0);
}

TEST(DestroyTest, DestroyNWithZeroCountDoesNothing)
{
    g_live = 0;
    alignas(Counter) unsigned char buf[sizeof(Counter)];
    Counter* p = ::new (static_cast<void*>(&buf[0])) Counter();
    EXPECT_EQ(g_live, 1);

    castle::memory::destroy_n(p, 0U);
    EXPECT_EQ(g_live, 1);

    castle::memory::destroy_at(p);
    EXPECT_EQ(g_live, 0);
}

} // namespace
