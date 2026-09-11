#include <gtest/gtest.h>

#include "castle/memory/lifetime.h"
#include "castle/memory/new.h"

namespace
{

struct Widget
{
    int value;
};

TEST(LifetimeTest, LaunderReturnsSamePointerValue)
{
    int x = 5;
    int* p = &x;
    EXPECT_EQ(castle::memory::launder(p), p);
}

TEST(LifetimeTest, LaunderConstOverload)
{
    CASTLE_CONST int x = 9;
    CASTLE_CONST int* p = &x;
    EXPECT_EQ(castle::memory::launder(p), p);
}

TEST(LifetimeTest, LaunderAllowsAccessToObjectInRawStorage)
{
    alignas(Widget) unsigned char buf[sizeof(Widget)];
    ::new (static_cast<void*>(&buf[0])) Widget{123};

    Widget* w = castle::memory::launder(reinterpret_cast<Widget*>(&buf[0]));
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->value, 123);

    w->~Widget();
}

} // namespace
