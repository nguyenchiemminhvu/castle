#include <gtest/gtest.h>
#include "castle/container/array.h"

namespace 
{
TEST(ArrayTest, FixedArrayAccessIteratorsAndReverse)
{
    constexpr castle::container::array<int,3> a(1,2,3);
    static_assert(a.size()==3 && a.capacity()==3 && !a.empty() && a.full(),"constexpr");
    EXPECT_EQ(a.front(),1);
    EXPECT_EQ(a.back(),3);
    EXPECT_EQ(a[1],2);
    EXPECT_EQ(a.data()[2],3);
    EXPECT_EQ(a.begin()+2,a.end()-1);
    EXPECT_EQ(a.cbegin()[0],1);
    EXPECT_EQ(a.cend(),a.end());
    int sum=0;
    for(auto x:a)
    {
        sum+=x;
    }
    EXPECT_EQ(sum,6);
    int rev=0;
    for(auto it=a.rbegin();it!=a.rend();++it)
    {
        rev=rev*10+*it;
    }
    EXPECT_EQ(rev,321);
    auto ri=a.rbegin();
    EXPECT_EQ(*ri,3);
    ++ri;
    EXPECT_EQ(*ri,2);
    --ri;
    EXPECT_EQ(*ri,3);
    castle::container::array<int,3> m(4,5,6);
    m[1]=9; EXPECT_EQ(m[1],9);
    auto cr=m.crbegin();
    EXPECT_EQ(*cr,6);
    auto rend=m.crend();
    EXPECT_EQ(rend==m.rend(),true);
}

TEST(ArrayTest, ZeroSpecialization)
{
    castle::container::array<int,0> z;
    EXPECT_EQ(z.size(),0U);
    EXPECT_EQ(z.capacity(),0U);
    EXPECT_TRUE(z.empty());
    EXPECT_TRUE(z.full());
    EXPECT_EQ(z.begin(),nullptr);
    EXPECT_EQ(z.end(),nullptr);
    EXPECT_EQ(z.data(),nullptr);
    EXPECT_EQ(z.rbegin(),z.rend());
    EXPECT_EQ(z.cbegin(),z.cend());
    EXPECT_EQ(z.crbegin(),z.crend());
}
}
