#include <gtest/gtest.h>

#include "castle/memory/object.h"

namespace
{

struct Widget
{
    int value;
};

TEST(ObjectTest, ObjectFromAddressYieldsTypedPointer)
{
    alignas(Widget) unsigned char buf[sizeof(Widget)];
    castle::memory::construct_at<Widget>(&buf[0], Widget{55});

    Widget* w = castle::memory::object_from_address<Widget>(&buf[0]);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(static_cast<void*>(w), static_cast<void*>(&buf[0]));
    EXPECT_EQ(w->value, 55);

    castle::memory::destroy_at(w);
}

TEST(ObjectTest, ObjectFromAddressConstOverload)
{
    alignas(Widget) unsigned char buf[sizeof(Widget)];
    castle::memory::construct_at<Widget>(&buf[0], Widget{99});

    CASTLE_CONST void* caddr = static_cast<CASTLE_CONST void*>(&buf[0]);
    CASTLE_CONST Widget* w = castle::memory::object_from_address<Widget>(caddr);
    ASSERT_NE(w, nullptr);
    EXPECT_EQ(w->value, 99);

    castle::memory::destroy_at(const_cast<Widget*>(w));
}

TEST(ObjectTest, ObjectFromAddressRoundTripScalar)
{
    alignas(int) unsigned char buf[sizeof(int)];
    castle::memory::construct_at<int>(&buf[0], 4321);

    int* p = castle::memory::object_from_address<int>(&buf[0]);
    EXPECT_EQ(*p, 4321);
}

} // namespace
