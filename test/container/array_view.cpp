#include <gtest/gtest.h>
#include "castle/container/array_view.h"
#include "castle/container/array.h"

namespace 
{
TEST(ArrayViewTest, PointerContainerAndSubview)
{
    int a[]={1,2,3,4};
    castle::container::array_view<int> v(a,4);
    EXPECT_EQ(v.size(),4U);
    EXPECT_FALSE(v.empty());
    EXPECT_EQ(v.data(),a);
    EXPECT_EQ(v[1],2);
    v[2]=8;
    EXPECT_EQ(a[2],8);
    EXPECT_EQ(v.front(),1);
    EXPECT_EQ(v.back(),4);
    EXPECT_EQ(v.end(),a+4);
    auto s=v.subview(1,2);
    EXPECT_EQ(s.size(),2U);
    EXPECT_EQ(s.front(),2);
    EXPECT_EQ(s.back(),8);
    auto z=v.subview(4,0);
    EXPECT_TRUE(z.empty());
    EXPECT_EQ(z.data(),v.data());
    auto bad=v.subview(5,1);
    EXPECT_TRUE(bad.empty());
    EXPECT_EQ(bad.data(),nullptr);
    const castle::container::array<int,3> ca(5,6,7);
    castle::container::array_view<const int> cv(ca);
    EXPECT_EQ(cv.size(),3U);
    EXPECT_EQ(cv[2],7);
    auto made=castle::container::make_array_view(a,4U);
    EXPECT_EQ(made.size(),4U);
    auto cmade=castle::container::make_array_view(static_cast<const int*>(a),4U);
    EXPECT_EQ(cmade.size(),4U);
    castle::container::array_view<int> empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.begin(),empty.end());
}
}
