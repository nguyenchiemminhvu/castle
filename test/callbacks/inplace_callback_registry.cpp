#include <gtest/gtest.h>
#include "castle/callbacks/inplace_callback_registry.h"

namespace
{
TEST(InplaceCallbackRegistryTest, StatefulCallbacksAndGeneration)
{
    using R=castle::callbacks::inplace_callback_registry<2,void(int),32,8>;
    R r;
    static_assert(R::capacity()==2,"cap");
    EXPECT_TRUE(r.empty());
    castle::status e=castle::status::unknown_error;
    auto bad=r.subscribe(R::callback_type{},&e);
    EXPECT_FALSE(bad.valid());
    EXPECT_EQ(e,castle::status::invalid_callback);
    int sum=0;
    auto a=r.subscribe([&](int v){sum+=v;},&e);
    auto b=r.subscribe([&](int v){sum+=2*v;});
    EXPECT_TRUE(a.valid());
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(r.size(),2U);
    EXPECT_EQ(e,castle::status::ok);
    r.invoke(3);
    EXPECT_EQ(sum,9);
    sum=0;
    r(2);
    EXPECT_EQ(sum,6);
    auto full=r.subscribe([&](int){},&e);
    EXPECT_FALSE(full.valid());
    EXPECT_EQ(e,castle::status::full);
    EXPECT_EQ(a.unsubscribe(),castle::status::ok);
    auto c=r.subscribe([&](int v){sum+=3*v;});
    EXPECT_TRUE(c.valid());
    EXPECT_EQ(c.index(),a.index());
    EXPECT_NE(c.generation(),a.generation());
    EXPECT_EQ(r.unsubscribe_slot(99,0),castle::status::invalid_subscription);
    EXPECT_EQ(r.unsubscribe_slot(c.index(),a.generation()),castle::status::invalid_subscription);
    r.clear();
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(b.unsubscribe(),castle::status::invalid_subscription);
    EXPECT_EQ(c.unsubscribe(),castle::status::invalid_subscription);
}
}
