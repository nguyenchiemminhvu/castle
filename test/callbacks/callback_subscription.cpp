#include <gtest/gtest.h>
#include "castle/callbacks/callback_subscription.h"

namespace
{
struct Owner : castle::callbacks::i_unsubscribable
{
    int calls=0;
    castle::size_type index=99;
    uint32_t generation=99;
    castle::status result=castle::status::ok;
    castle::status unsubscribe_slot(castle::size_type i,uint32_t g) noexcept override
    {
        ++calls;
        index=i;
        generation=g;
        return result;
    }
};

TEST(CallbackSubscriptionTest, DefaultResetAndSuccessfulUnsubscribe)
{
    castle::callbacks::callback_subscription s;
    EXPECT_FALSE(s.valid());
    EXPECT_FALSE(static_cast<bool>(s));
    EXPECT_EQ(s.index(),0U);
    EXPECT_EQ(s.generation(),0U);
    EXPECT_EQ(s.unsubscribe(),castle::status::invalid_subscription);
    Owner o;
    castle::callbacks::callback_subscription live(&o,3,42);
    EXPECT_TRUE(live.valid());
    EXPECT_TRUE(static_cast<bool>(live));
    EXPECT_EQ(live.index(),3U);
    EXPECT_EQ(live.generation(),42U);
    EXPECT_EQ(live.unsubscribe(),castle::status::ok);
    EXPECT_EQ(o.calls,1);
    EXPECT_EQ(o.index,3U);
    EXPECT_EQ(o.generation,42U);
    EXPECT_FALSE(live.valid());
    EXPECT_EQ(live.unsubscribe(),castle::status::invalid_subscription);
}

TEST(CallbackSubscriptionTest, ResetAfterOwnerFailure)
{
    Owner o;
    o.result=castle::status::invalid_subscription;
    castle::callbacks::callback_subscription s(&o,1,9);
    EXPECT_EQ(s.unsubscribe(),castle::status::invalid_subscription);
    EXPECT_EQ(o.calls,1);
    EXPECT_FALSE(s.valid());
    s.reset();
    EXPECT_FALSE(s.valid());
}
}
