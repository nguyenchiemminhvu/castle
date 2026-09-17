#include <gtest/gtest.h>
#include "castle/events/sigslot.h"

namespace
{
struct Obj
{
    int sum=0;
    void add(int v)
    {
        sum+=v;
    }
    int read=0;
    void add_const(int v) const
    {
        const_cast<Obj*>(this)->read+=v;
    }
};

TEST(SigslotTest, ConnectEmitDisconnectMoveAndFull)
{
    using S=castle::sigslot::signal<3,void(int)>;

    S s;
    int a=0,b=0;
    auto c1=s.connect([&](int v){a+=v;});
    auto c2=s.connect([&](int v){b+=2*v;});
    Obj o;
    auto c3=s.connect(o,&Obj::add);
    EXPECT_TRUE(c1.connected());
    EXPECT_TRUE(c2);
    s.emit(3);
    EXPECT_EQ(a,3);
    EXPECT_EQ(b,6);
    EXPECT_EQ(o.sum,3);
    c1.disconnect();
    EXPECT_FALSE(c1.connected());
    EXPECT_EQ(c1.disconnect(),castle::sigslot::signal_error::invalid_connection);
    s(2);
    EXPECT_EQ(a,3);
    EXPECT_EQ(a,3);
    EXPECT_EQ(b,10);
    EXPECT_EQ(o.sum,5);
    S::connection_type moved(std::move(c2));
    EXPECT_FALSE(c2.connected());
    EXPECT_TRUE(moved.connected());
    S::connection_type assigned;
    assigned=std::move(moved);
    EXPECT_FALSE(moved.connected());
    EXPECT_TRUE(assigned.connected());
    assigned.disconnect();
    auto f1=s.connect([](int){});
    auto f2=s.connect([](int){});
    auto f3=s.connect([](int){});
    EXPECT_TRUE(f1.connected()||f2.connected()||f3.connected());
    auto full=s.connect([](int){});
    EXPECT_FALSE(full.connected());
    s.disconnect_all();
    EXPECT_FALSE(c3.connected());
    EXPECT_FALSE(f1.connected());
    EXPECT_FALSE(full.connected());
}

TEST(SigslotTest, InvalidCallbackAndConnectionLifecycle)
{
    using S=castle::sigslot::signal<2,void(int)>;

    S::connection_type invalid;
    EXPECT_FALSE(invalid.connected());
    EXPECT_EQ(invalid.disconnect(),castle::sigslot::signal_error::invalid_connection);
    S s;
    S::callback_type empty;
    auto c=s.connect(std::move(empty));
    EXPECT_FALSE(c.connected());
    EXPECT_EQ(s.size(),0U);
    S::connection_type stale;
    {
        S inner;
        stale=inner.connect([](int){});
        EXPECT_TRUE(stale.connected());
    }
    EXPECT_FALSE(stale.connected());
}

TEST(SigslotTest, ConstMemberFunctionAndReadyMadeCallback)
{
    using S=castle::sigslot::signal<2,void(int)>;
    const Obj o{};
    S s;
    auto c=s.connect(o,&Obj::add_const);
    EXPECT_TRUE(c.connected());
    s(4);
    EXPECT_EQ(o.read,4);
    S::callback_type cb([](int){});
    auto d=s.connect(std::move(cb));
    EXPECT_TRUE(d.connected());
    s.disconnect_all();
}
}