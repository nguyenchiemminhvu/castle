#include <gtest/gtest.h>
#include "castle/design_patterns/observer.h"

namespace
{
struct IntObs: castle::design_patterns::observer<int>
{
    int sum=0;
    void notify(const int&v) override
    {
        sum+=v;
    }
};

struct VoidObs: castle::design_patterns::observer<void>
{
    int calls=0;
    void notify() override
    {
        ++calls;
    }
};

TEST(ObserverTest, AddRemoveNotifyCapacityAndReuse)
{
    castle::design_patterns::observable<IntObs,2> o;
    IntObs a,b;
    EXPECT_FALSE(o.add_observer(nullptr));
    EXPECT_TRUE(o.add_observer(&a));
    EXPECT_FALSE(o.add_observer(&a));
    EXPECT_TRUE(o.add_observer(&b));
    EXPECT_FALSE(o.add_observer(&a));
    int x=3;
    o.notify_observers(x);
    EXPECT_EQ(a.sum,3);
    EXPECT_EQ(b.sum,3);
    EXPECT_FALSE(o.remove_observer(nullptr));
    EXPECT_TRUE(o.remove_observer(&a));
    EXPECT_FALSE(o.remove_observer(&a));
    EXPECT_TRUE(o.add_observer(&a));
    EXPECT_TRUE(o.remove_observer(&b));
    EXPECT_TRUE(o.remove_observer(&a));
}

TEST(ObserverTest, VoidSpecializationAndTypedInheritance)
{
    castle::design_patterns::observable<VoidObs,1> o;
    VoidObs v;
    EXPECT_TRUE(o.add_observer(&v));
    o.notify_observers();
    EXPECT_EQ(v.calls,1);
    EXPECT_FALSE(o.add_observer(&v));
    EXPECT_TRUE(o.remove_observer(&v));
}
} // namespace
