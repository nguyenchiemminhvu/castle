#include <gtest/gtest.h>
#include "castle/callbacks/inplace_function.h"

namespace 
{
struct Stateful
{
    int bias;
    int operator()(int x) { return x+bias; }
};

TEST(InplaceFunctionTest, EmptyInvokeCopyMoveAndAssignment)
{
    using F=castle::callbacks::inplace_function<int(int),32,8>;
    F empty;
    EXPECT_FALSE(static_cast<bool>(empty));
    F f(Stateful{3});
    EXPECT_TRUE(static_cast<bool>(f));
    EXPECT_EQ(f(4),7);
    const F& cf=f;
    EXPECT_EQ(cf(5),8);
    F copy(f);
    EXPECT_TRUE(copy);
    EXPECT_EQ(copy(6),9);
    F moved(std::move(copy));
    EXPECT_TRUE(moved);
    EXPECT_FALSE(copy);
    EXPECT_EQ(moved(7),10);
    F assigned(Stateful{1});
    assigned= f;
    EXPECT_EQ(assigned(2),5);
    F move_assigned(Stateful{9});
    move_assigned=std::move(assigned);
    EXPECT_EQ(move_assigned(2),5);
    EXPECT_FALSE(assigned);
    F self(Stateful{4});
    self=self;
    EXPECT_EQ(self(1),5);
    F& self_alias=self;
    self=std::move(self_alias);
    EXPECT_EQ(self(2),6);
}

TEST(InplaceFunctionTest, VoidAndDestructionPaths)
{
    using F=castle::callbacks::inplace_function<void(int),32,8>;
    int sum=0;
    {
        F f([&](int x){sum+=x;});
        F c=f;
        c(2);
        F m(std::move(c));
        m(3);
        F a;
        a=std::move(m);
        a(4);
    }
    EXPECT_EQ(sum,9);
}
}
