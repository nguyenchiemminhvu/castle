#include <gtest/gtest.h>
#include "castle/callbacks/delegate.hpp"

namespace
{

int g_value=0;

int add(int a,int b)
{
    return a+b;
}

void take(int v)
{
    g_value+=v;
}

struct Fun
{
    int operator()(int a,int b) const
    {
        return a*b;
    }
};

struct Obj
{
    int value=0;
    int add(int v)
    {
        value+=v;
        return value;
    }
    int read(int v) const
    {
        return v+7;
    }
};

Obj global_obj;

void static_void(int v)
{
    g_value+=v;
}

TEST(FunctionTest, FreeFunctionAndFunctorVariants)
{
    castle::callbacks::delegate_ptr<int(int,int)> f(&add);
    EXPECT_EQ(f(2,3),5);
    static_assert(castle::meta::is_same<decltype(f)::return_type,int>::value,"ret");
    auto ff=castle::callbacks::make_delegate_ft<int(int,int)>([](int a,int b){return a-b;});
    EXPECT_EQ(ff(5,2),3);
    Fun fun;
    castle::callbacks::delegate_ft<Fun,int(int,int)> ff2(fun);
    EXPECT_EQ(ff2(3,4),12);
    castle::callbacks::delegate_ftr<Fun,int(int,int)> fr(fun);
    EXPECT_EQ(fr(2,6),12);
    auto fr2=castle::callbacks::make_delegate_ftr<int(int,int)>(fun);
    EXPECT_EQ(fr2(4,5),20);
    castle::callbacks::delegate_ptr<void(int)> fv(&take);
    g_value=0;
    fv(4);
    EXPECT_EQ(g_value,4);
    castle::callbacks::delegate_ptr<void(int)> fv2(&static_void);
    fv2(3);
    EXPECT_EQ(g_value,7);
}

TEST(FunctionTest, MemberAndCompileTimeBoundVariants)
{
    Obj o;
    castle::callbacks::delegate_member<Obj,int(int)> fm(o,&Obj::add);
    EXPECT_EQ(fm(3),3);
    castle::callbacks::delegate_ptr_ct<&add> ct;
    EXPECT_EQ(ct(7,8),15);
    castle::callbacks::delegate_ft_ct<Fun,int(int,int)> ctf;
    EXPECT_EQ(ctf(3,5),15);
    castle::callbacks::delegate_member_ct<&Obj::add> ctm(o);
    EXPECT_EQ(ctm(2),5);
    const Obj co{};
    castle::callbacks::delegate_member_ct<&Obj::read> ctmc(co);
    EXPECT_EQ(ctmc(5),12);
    castle::callbacks::delegate_ins_ct<global_obj,&Obj::add> ctim;
    EXPECT_EQ(ctim(4),4);
    EXPECT_EQ(global_obj.value,4);
    castle::callbacks::delegate_ins_ct<global_obj,&Obj::read> ctimc;
    EXPECT_EQ(ctimc(5),12);
}
}
