#include <gtest/gtest.h>
#include "castle/design_patterns/visitor.h"

namespace
{
struct A;
struct B;

using V=castle::design_patterns::visitor<A&,B&>;

struct A: castle::design_patterns::visitable<V>
{
    int n=0;
    void accept(V& v) override
    {
        v.visit(*this);
    }
};

struct B: castle::design_patterns::visitable<V>
{
    int n=0;
    void accept(V& v) override
    {
        v.visit(*this);
    }
};

struct AV: V
{
    int a=0,b=0;
    void visit(A& x) override
    {
        ++a;
        x.n+=1;
    }
    void visit(B& x) override
    {
        ++b;
        x.n+=2;
    }
};

TEST(VisitorTest, MultiTypeDispatch)
{
    A a;
    B b;
    AV v;
    a.accept(v);
    b.accept(v);
    EXPECT_EQ(v.a,1);
    EXPECT_EQ(v.b,1);
    EXPECT_EQ(a.n,1);
    EXPECT_EQ(b.n,2);
}
} // namespace
