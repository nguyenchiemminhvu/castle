#include <gtest/gtest.h>
#include "castle/events/event_dispatcher.h"

namespace
{
struct A{};
struct B{};

int ga=0, gb=0;

void fa(int v)
{
    ga+=v;
}

void fb()
{
    ++gb;
}

TEST(EventDispatcherTest, RegistrationDispatchCapacityClear)
{
    using D=castle::events::event_dispatcher<
        castle::events::event_config<A,2,void(int)>,
        castle::events::event_config<B,1,void()>>;

    static_assert(D::event_capacity()==2U,"events");
    static_assert(D::callback_capacity<A>()==2U,"cap");
    D d;
    castle::callbacks::function<void(int)> ca(&fa);
    castle::callbacks::function<void()> cb(&fb);
    castle::status e=castle::status::unknown_error;
    auto s1=d.register_callback<A>(&ca,&e);
    EXPECT_TRUE(s1.valid());
    EXPECT_TRUE(s1.valid());
    EXPECT_EQ(e,castle::status::ok);
    auto s2=d.register_callback<A>(&ca);
    EXPECT_TRUE(s2.valid());
    EXPECT_EQ(d.subscriber_count<A>(),2U);
    auto sf=d.register_callback<A>(&ca,&e);
    EXPECT_FALSE(sf.valid());
    EXPECT_EQ(e,castle::status::full);
    auto sb=d.register_callback<B>(&cb);
    EXPECT_TRUE(sb.valid());
    ga=gb=0;
    EXPECT_EQ(d.dispatch_event<A>(3),castle::status::ok);
    EXPECT_EQ(ga,6);
    d.dispatch_event<B>();
    EXPECT_EQ(gb,1);
    d.clear_event<A>();
    EXPECT_EQ(d.subscriber_count<A>(),0U);
    d.clear();
    EXPECT_EQ(d.subscriber_count<B>(),0U);
    EXPECT_EQ(s1.unsubscribe(),castle::status::invalid_subscription);
}
}
