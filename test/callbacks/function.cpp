#include <gtest/gtest.h>
#include "castle/callbacks/function.hpp"

namespace 
{
struct Stateful
{
    int bias;
    int operator()(int x) { return x+bias; }
};

void foo(int& val)
{
    val++;
}

TEST(FunctionTest, FooIncrements)
{
    int val=0;
    castle::callbacks::function<void(int&)> f(foo);
    castle::callbacks::function<void(int&),32,8> ff=foo;
    f(val);
    ff(val);
    EXPECT_EQ(val,2);
}

TEST(FunctionTest, EmptyInvokeCopyMoveAndAssignment)
{
    using F=castle::callbacks::function<int(int),32,8>;
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

TEST(FunctionTest, VoidAndDestructionPaths)
{
    using F=castle::callbacks::function<void(int),32,8>;
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

int AddOne(int v)
{
return v + 1;
}

TEST(FunctionTest, EmptyCopyAndMoveConstructorBranches)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F empty;

    F copy(empty);
    EXPECT_FALSE(copy);

    F moved(std::move(empty));
    EXPECT_FALSE(moved);
    EXPECT_FALSE(empty);
}

TEST(FunctionTest, EmptyCopyAndMoveAssignmentBranches)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F src_empty;

    F dst(Stateful{10});
    EXPECT_TRUE(dst);

    dst = src_empty;
    EXPECT_FALSE(dst);

    F dst2(Stateful{20});
    dst2 = std::move(src_empty);

    EXPECT_FALSE(dst2);
    EXPECT_FALSE(src_empty);
}

TEST(FunctionTest, FunctionPointerCopyCtorExecutesPointerCopyLambda)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F original(&AddOne);
    F copy(original);

    EXPECT_TRUE(copy);
    EXPECT_EQ(copy(5), 6);
}

TEST(FunctionTest, FunctionPointerMoveCtorExecutesPointerMoveLambda)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F original(&AddOne);
    F moved(std::move(original));

    EXPECT_TRUE(moved);
    EXPECT_FALSE(original);
    EXPECT_EQ(moved(10), 11);
}

TEST(FunctionTest, FunctionPointerCopyAssignmentExecutesPointerCopyLambda)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F src(&AddOne);
    F dst;

    dst = src;

    EXPECT_TRUE(dst);
    EXPECT_EQ(dst(20), 21);
}

TEST(FunctionTest, FunctionPointerMoveAssignmentExecutesPointerMoveLambda)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F src(&AddOne);
    F dst;

    dst = std::move(src);

    EXPECT_TRUE(dst);
    EXPECT_FALSE(src);
    EXPECT_EQ(dst(30), 31);
}

TEST(FunctionTest, NullptrAssignmentResetsObject)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F f(&AddOne);

    EXPECT_TRUE(f);

    f = nullptr;

    EXPECT_FALSE(f);
}

int MultiplyBy2(int v)
{
    return v * 2;
}

TEST(FunctionTest, CallbackPointerAssignmentNonNullBranch)
{
    using F = castle::callbacks::function<int(int), 32, 8>;

    F f(Stateful{5}); // ensure destroy_ptr_ is already valid

    EXPECT_EQ(f(1), 6);

    f = &MultiplyBy2; // exercise operator=(callback_ptr_t) non-null path

    EXPECT_TRUE(f);
    EXPECT_EQ(f(3), 6);

    F copy(f); // execute callback_ptr copy lambda
    EXPECT_EQ(copy(4), 8);

    F moved(std::move(f)); // execute callback_ptr move lambda
    EXPECT_EQ(moved(5), 10);
    EXPECT_FALSE(f);
}

}
