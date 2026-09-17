#include <gtest/gtest.h>
#include "castle/callbacks/exec_policy.hpp"

namespace
{
struct FakeClock
{
    using duration=castle::chrono::milliseconds;
    using time_point=castle::chrono::time_point<FakeClock,duration>;
    static time_point now() noexcept
    {
        return time_point(duration(now_ms));
    }
    static int64_t now_ms;
};

int64_t FakeClock::now_ms=0;

namespace policy = castle::callbacks::policy;

TEST(CallbackPolicyTest, OnceEveryNAndOnChange)
{
    int hits=0;
    policy::once p{[&](int x){hits+=x;}};
    EXPECT_FALSE(p.has_fired());
    p.execute(2);
    EXPECT_TRUE(p.has_fired());
    p(3);
    EXPECT_EQ(hits,2);
    p.reset();
    EXPECT_FALSE(p.has_fired());
    p(4);
    EXPECT_EQ(hits,6);
    hits=0;
    auto pc=policy::make_once<policy::concurrent>([&](int x){hits+=x;});
    pc(2);
    pc(3);
    EXPECT_EQ(hits,2);
    pc.reset();
    pc(5);
    EXPECT_EQ(hits,7);
    hits=0;
    policy::every_n en{3,[&](){++hits;}};
    EXPECT_EQ(en.interval(),3U);
    en();
    en();
    EXPECT_EQ(hits,0);
    en();
    EXPECT_EQ(hits,1);
    en.reset();
    en();
    en();
    en();
    EXPECT_EQ(hits,2);
    auto enc=policy::make_every_n<policy::concurrent>(2,[&](){++hits;});
    enc();
    enc();
    enc();
    enc();
    EXPECT_EQ(hits,4);
    enc.reset();
    enc();
    enc();
    EXPECT_EQ(hits,5);
    auto ect=policy::make_every_n_ct<2>([&](){++hits;});
    ect();
    ect();
    EXPECT_EQ(hits,6);
    int last=0,count=0;
    auto oc=policy::make_on_change<int>([&](int v){last=v;++count;});
    EXPECT_FALSE(oc.has_value());
    oc(3);
    oc(3);
    oc(4);
    EXPECT_EQ(count,2);
    EXPECT_EQ(last,4);
    oc.reset();
    EXPECT_FALSE(oc.has_value());
    oc(4);
    EXPECT_EQ(count,3);
    policy::on_change oci{9,[&](int v){last=v;++count;}};
    EXPECT_TRUE(oci.has_value());
    oci(9);
    EXPECT_EQ(count,3);
    oci(10);
    EXPECT_EQ(count,4);
}

TEST(CallbackPolicyTest, ArmedThrottlePeriodicWithControlledClock)
{
    using namespace castle::chrono::literals::chrono_literals;
    FakeClock::now_ms=0;
    int hits=0;
    auto aw=policy::make_armed_window<policy::single_thread, FakeClock>(10_ms,[&](){++hits;});
    EXPECT_TRUE(aw.armed());
    EXPECT_EQ(aw.window().count(),10);
    aw.execute();
    EXPECT_TRUE(aw.fired());
    EXPECT_EQ(hits,1);
    aw.execute();
    EXPECT_EQ(hits,1);
    aw.reset();
    EXPECT_TRUE(aw.armed());
    FakeClock::now_ms=11;
    aw.execute();
    EXPECT_TRUE(aw.expired());
    EXPECT_EQ(hits,1);
    aw.rearm(5_ms);
    EXPECT_TRUE(aw.armed());
    aw.expire();
    EXPECT_TRUE(aw.expired());
    aw.execute();
    EXPECT_EQ(hits,1);
    EXPECT_GE(aw.deadline().time_since_epoch().count(),11);
    FakeClock::now_ms=20;
    auto ac=policy::make_armed_window<policy::concurrent, FakeClock>(5_ms,[&](){++hits;});
    ac();
    EXPECT_TRUE(ac.fired());
    ac.reset();
    EXPECT_TRUE(ac.armed());
    FakeClock::now_ms=30;
    ac.execute();
    EXPECT_TRUE(ac.expired());
    ac.rearm(2_ms);
    ac.expire();
    EXPECT_TRUE(ac.expired());
    FakeClock::now_ms=100;
    hits=0;
    auto th=policy::make_throttle<policy::single_thread, FakeClock>(10_ms,[&](){++hits;});
    EXPECT_EQ(th.interval().count(),10);
    th();
    th();
    EXPECT_EQ(hits,1);
    FakeClock::now_ms=110;
    th();
    EXPECT_EQ(hits,2);
    th.reset();
    th();
    EXPECT_EQ(hits,3);
    FakeClock::now_ms=200;
    hits=0;
    auto per=policy::make_periodic<policy::single_thread, FakeClock>(10_ms,[&](){++hits;});
    EXPECT_EQ(per.period().count(),10);
    per.poll();
    EXPECT_EQ(hits,0);
    FakeClock::now_ms=220;
    per.poll();
    EXPECT_EQ(hits,2);
    FakeClock::now_ms=251;
    per();
    EXPECT_EQ(hits,5);
    per.reset();
    EXPECT_GT(per.next_deadline().time_since_epoch().count(),251);
}
}
