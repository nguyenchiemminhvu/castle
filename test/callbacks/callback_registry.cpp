#include <gtest/gtest.h>
#include "castle/callbacks/callback_registry.h"

namespace
{
volatile int sum=0;

void add1(int v)
{
    sum+=v;
}

void add2(int v)
{
    sum+=2*v;
}

TEST(CallbackRegistryTest, SubscribeInvokeUnsubscribeReuseAndCapacity)
{
    using R=castle::callbacks::callback_registry<2,void(int)>;
    R r;
    EXPECT_EQ(R::capacity(),2U);
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.size(),0U);
    auto empty_sub=r.subscribe(nullptr);
    EXPECT_FALSE(empty_sub.valid());
    castle::status e=castle::status::unknown_error; auto bad=r.subscribe(nullptr,&e); EXPECT_FALSE(bad.valid());
    EXPECT_EQ(e,castle::status::invalid_callback);
    castle::callbacks::function<void(int)> f1(&add1), f2(&add2);
    auto s1=r.subscribe(&f1,&e);
    auto s1copy=s1;
    EXPECT_TRUE(s1.valid());
    EXPECT_EQ(e,castle::status::ok);
    auto s2=r.subscribe(&f2);
    EXPECT_TRUE(s2);
    EXPECT_EQ(r.size(),2U);
    sum=0; r.invoke(3);
    EXPECT_EQ(sum,9); sum=0; r(2);
    EXPECT_EQ(sum,6);
    auto full=r.subscribe(&f1,&e);
    EXPECT_FALSE(full.valid());
    EXPECT_EQ(e,castle::status::full);
    EXPECT_EQ(s1.unsubscribe(),castle::status::ok);
    EXPECT_EQ(s1copy.unsubscribe(),castle::status::invalid_subscription);
    EXPECT_EQ(r.size(),1U);
    auto s3=r.subscribe(&f1);
    EXPECT_TRUE(s3.valid());
    EXPECT_EQ(s3.index(),0U);
    EXPECT_NE(s3.generation(),s1.generation());
    EXPECT_EQ(r.unsubscribe_slot(99,0),castle::status::invalid_subscription);
    EXPECT_EQ(r.unsubscribe_slot(1,99),castle::status::invalid_subscription);
    EXPECT_EQ(r.unsubscribe_slot(0,s1.generation()),castle::status::invalid_subscription);
    r.clear();
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(s2.unsubscribe(),castle::status::invalid_subscription);
    EXPECT_EQ(s3.unsubscribe(),castle::status::invalid_subscription);
}
}
