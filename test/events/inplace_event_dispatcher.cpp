#include <gtest/gtest.h>
#include "castle/events/inplace_event_dispatcher.h"

namespace
{
struct A{};
struct B{};

TEST(InplaceEventDispatcherTest, StatefulCallbacksAndCapacities)
{
    using D=castle::events::inplace_event_dispatcher<
        castle::events::event_config<A,2,void(int),32,8>,
        castle::events::event_config<B,1,void(),16,8>>;

    static_assert(D::event_capacity()==2U,"events");
    static_assert(D::callback_capacity<A>()==2U,"cap");
    static_assert(D::callback_storage_size<A>()==32U,"size");
    static_assert(D::callback_storage_alignment<B>()==8U,"alignment");
    
    D d;
    int sum=0,calls=0;
    castle::status e=castle::status::unknown_error;
    auto s=d.register_callback<A>([&](int x){sum+=x;},&e);
    EXPECT_TRUE(s.valid());
    EXPECT_EQ(e,castle::status::ok);
    auto s2=d.register_callback<A>([&](int x){sum+=2*x;});
    EXPECT_TRUE(s2.valid());
    auto full=d.register_callback<A>([&](int){} ,&e);
    EXPECT_FALSE(full.valid());
    EXPECT_EQ(e,castle::status::full);
    auto vb=d.register_callback<B>([&]{++calls;});
    EXPECT_TRUE(vb.valid());
    d.dispatch_event<A>(3);
    d.dispatch_event<B>();
    EXPECT_EQ(sum,9);
    EXPECT_EQ(calls,1);
    EXPECT_EQ(d.subscriber_count<A>(),2U);
    d.clear_event<A>();
    EXPECT_EQ(d.subscriber_count<A>(),0U);
    d.clear();
    EXPECT_EQ(d.subscriber_count<B>(),0U);
}
}
