#include <gtest/gtest.h>

#include "castle/atomic/atomic.h"

namespace
{
struct Pair
{
    int a;
    int b;
    bool operator==(const Pair& o) const
    {
        return a==o.a && b==o.b;
    }
};

TEST(AtomicTest, IntegralOperationsAndMemoryOrders)
{
    castle::atomic<int> a;
    static_assert(noexcept(castle::atomic<int>()),"ctor");
    EXPECT_EQ(a.load(),0); a.store(3,castle::memory_order_relaxed);
    EXPECT_EQ((int)a,3);
    EXPECT_EQ(a.exchange(5),3);
    EXPECT_EQ(a.load(),5);
    EXPECT_EQ(a++,5);
    EXPECT_EQ((int)a,6);
    EXPECT_EQ(++a,7);
    EXPECT_EQ(a--,7);
    EXPECT_EQ(--a,5);
    EXPECT_EQ((a+=4),9);
    EXPECT_EQ((a-=2),7);
    EXPECT_EQ((a&=3),3);
    EXPECT_EQ((a|=8),11);
    EXPECT_EQ((a^=2),9);
    EXPECT_EQ(a.fetch_add(2),9);
    EXPECT_EQ(a.load(),11);
    EXPECT_EQ(a.fetch_sub(1),11);
    EXPECT_EQ(a.fetch_and(7),10);
    EXPECT_EQ(a.fetch_or(16),2);
    EXPECT_EQ(a.fetch_xor(3),18);
    int expected=17;
    EXPECT_TRUE(a.compare_exchange_strong(expected,17));
    EXPECT_EQ(a.load(),17);
    expected=1;
    EXPECT_FALSE(a.compare_exchange_strong(expected,2));
    EXPECT_EQ(expected,17);
    expected=17;
    EXPECT_TRUE(a.compare_exchange_weak(expected,19,castle::memory_order_acq_rel,castle::memory_order_acquire));
    EXPECT_EQ(a.load(),19);
    expected=1;
    EXPECT_FALSE(a.compare_exchange_weak(expected,20));
    EXPECT_EQ(expected,19);
    EXPECT_TRUE(a.is_lock_free() || !a.is_lock_free());
    volatile castle::atomic<int> va(1);
    va.store(2);
    EXPECT_EQ(va.load(),2);
    EXPECT_EQ(va++,2);
    EXPECT_EQ(++va,4);
    EXPECT_EQ(va.fetch_add(1),4);
    EXPECT_EQ(va.fetch_sub(1),5);
    EXPECT_EQ(va.exchange(9),4);
    int ve=9;
    EXPECT_TRUE(va.compare_exchange_strong(ve,10));
    EXPECT_EQ(va.load(),10);
    ve=0;
    EXPECT_FALSE(va.compare_exchange_weak(ve,1));
    EXPECT_EQ(ve,10);
    castle::atomic_thread_fence(castle::memory_order_seq_cst);
    castle::atomic_signal_fence(castle::memory_order_seq_cst);
}

TEST(AtomicTest, PointerSpecializationAndFallback)
{
    int data[4]={1,2,3,4};
    castle::atomic<int*> p;
    EXPECT_EQ(p.load(),nullptr);
    p.store(data); EXPECT_EQ(*p,1);
    EXPECT_EQ(p.exchange(data+1),data);
    EXPECT_EQ(p.load(),data+1);
    EXPECT_EQ(p++,data+1);
    EXPECT_EQ(p.load(),data+2);
    EXPECT_EQ(++p,data+3);
    EXPECT_EQ(p--,data+3);
    EXPECT_EQ(p.load(),data+2);
    EXPECT_EQ(--p,data+1);
    EXPECT_EQ((p+=2),data+3);
    EXPECT_EQ((p-=1),data+2);
    EXPECT_EQ(p.fetch_add(1),data+2);
    EXPECT_EQ(p.load(),data+3);
    EXPECT_EQ(p.fetch_sub(2),data+3);
    EXPECT_EQ(p.load(),data+1);
    int* expected=data+1;
    EXPECT_TRUE(p.compare_exchange_strong(expected,data));
    EXPECT_EQ(p.load(),data);
    expected=data+3;
    EXPECT_FALSE(p.compare_exchange_weak(expected,data+2));
    EXPECT_EQ(expected,data);
    volatile castle::atomic<int*> vp(data);
    EXPECT_EQ(vp.load(),data);
    vp+=1;
    EXPECT_EQ(vp.load(),data+1);
    vp-=1;
    EXPECT_EQ(vp.load(),data);
    vp.store(data+2);
    EXPECT_EQ(vp.exchange(data+3),data+2);
    int* ve=data+3;
    EXPECT_TRUE(vp.compare_exchange_strong(ve,data));
    castle::atomic<Pair> x(Pair{1,2});
    EXPECT_FALSE(x.is_lock_free());
    EXPECT_EQ(x.load(),(Pair{1,2}));
    x.store(Pair{3,4});
    EXPECT_EQ(x.load(),(Pair{3,4}));
    EXPECT_EQ(x.exchange(Pair{5,6}), (Pair{3,4}));
    Pair e{5,6};
    EXPECT_TRUE(x.compare_exchange_strong(e,Pair{7,8}));
    e={1,1};
    EXPECT_FALSE(x.compare_exchange_weak(e,Pair{9,9}));
    EXPECT_EQ(e,(Pair{7,8}));
}
}
