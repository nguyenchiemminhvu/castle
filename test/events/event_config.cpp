#include <gtest/gtest.h>
#include "castle/events/event_config.h"

namespace
{
struct A{};

TEST(EventConfigTest, CompileTimeTraits)
{
    using C=castle::events::event_config<A,3,void(int),32,16>;

    static_assert(castle::meta::is_same<C::event_tag,A>::value,"tag");
    static_assert(C::max_callback==3U,"callbacks");
    static_assert(C::callback_storage_size==32U,"size");
    static_assert(C::callback_storage_alignment==16U,"alignment");
    EXPECT_EQ(C::max_callback,3U);
    EXPECT_EQ(C::callback_storage_size,32U);
    EXPECT_EQ(C::callback_storage_alignment,16U);
}
}
