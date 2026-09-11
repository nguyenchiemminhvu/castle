#include <gtest/gtest.h>

#include "castle/algorithm/algorithm.h"
#include "castle/container/array.h"
#include "castle/container/forward_list.h"

namespace
{
TEST(AlgorithmTest, FindCountPredicatesAndEqual)
{
    int a[]={1,2,2,4};
    int b[]={1,2,3,4};
    EXPECT_EQ(castle::find(a,a+4,2),a+1);
    EXPECT_EQ(castle::find(a,a+4,9),a+4);
    EXPECT_EQ(castle::find_if(a,a+4,[](int x){return x>3;}),a+3);
    EXPECT_EQ(castle::find_if_not(a,a+4,[](int x){return x<4;}),a+3);
    EXPECT_EQ(castle::count(a,a+4,2),2U);
    EXPECT_EQ(castle::count_if(a,a+4,[](int x){return (x%2)==0;}),3U);
    EXPECT_TRUE(castle::all_of(a,a+4,[](int x){return x>0;}));
    EXPECT_FALSE(castle::all_of(a,a+4,[](int x){return x<3;}));
    EXPECT_TRUE(castle::any_of(a,a+4,[](int x){return x==4;}));
    EXPECT_FALSE(castle::any_of(a,a+4,[](int x){return x==9;}));
    EXPECT_TRUE(castle::none_of(a,a+4,[](int x){return x==9;}));
    EXPECT_FALSE(castle::none_of(a,a+4,[](int x){return x==2;}));
    EXPECT_TRUE(castle::equal(a,a+4,a));
    EXPECT_FALSE(castle::equal(a,a+4,b));
    EXPECT_TRUE(castle::equal(a,a+4,b,[](int x,int y){return x==y || (x==2&&y==3);}));
    EXPECT_FALSE(castle::equal(a,a+4,b,[](int x,int y){return x==y;}));
    EXPECT_TRUE(castle::equal(a,a+4,a+0,a+4));
    EXPECT_FALSE(castle::equal(a,a+4,a,a+3));
    EXPECT_FALSE(castle::equal(a,a+3,a,a+4));
    auto m1=castle::mismatch(a,a+4,b);
    EXPECT_EQ(m1.first,a+2);
    EXPECT_EQ(m1.second,b+2);
    auto m2=castle::mismatch(a,a+4,b,[](int x,int y){return x==y;});
    EXPECT_EQ(m2.first,a+2);
    auto m3=castle::mismatch(a,a+4,b,b+4);
    EXPECT_EQ(m3.first,a+2);
    EXPECT_EQ(m3.second,b+2);
    auto m4=castle::mismatch(a,a+4,b,b+4,[](int x,int y){return x==y;});
    EXPECT_EQ(m4.first,a+2);
}

TEST(AlgorithmTest, CopyFillTransformReplaceRemoveUnique)
{
    int a[]={1,2,2,4};
    int out[4]={};
    int out2[4]={};
    EXPECT_EQ(castle::copy(a,a+4,out),out+4);
    EXPECT_EQ(out[2],2);
    EXPECT_EQ(castle::copy_n(a,2,out2),out2+2);
    EXPECT_EQ(out2[1],2);
    int evens[4]={};
    EXPECT_EQ(castle::copy_if(a,a+4,evens,[](int x){return (x%2)==0;}),evens+3);
    int moved[4]={};
    EXPECT_EQ(castle::move(a,a+4,moved),moved+4);
    castle::fill(moved,moved+4,7);
    for(int x:moved)
    {
        EXPECT_EQ(x,7);
    }
    EXPECT_EQ(castle::fill_n(moved,2,3),moved+2);
    EXPECT_EQ(moved[0],3);
    EXPECT_EQ(moved[1],3);
    int unary[4]={};
    EXPECT_EQ(castle::transform(moved,moved+4,unary,[](int x){return x+1;}),unary+4);
    EXPECT_EQ(unary[0],4);
    int rhs[]={10,20,30,40};
    int binary[4]={};
    EXPECT_EQ(castle::transform(moved,moved+4,rhs,binary,[](int x,int y){return x+y;}),binary+4);
    EXPECT_EQ(binary[0],13);
    int g=0;
    castle::generate(binary,binary+4,[&](){return ++g;});
    EXPECT_EQ(binary[0],1);
    EXPECT_EQ(binary[3],4);
    castle::replace(binary,binary+4,2,9);
    EXPECT_EQ(binary[1],9);
    castle::replace_if(binary,binary+4,[](int x){return x%2==0;},8);
    EXPECT_EQ(binary[3],8);
    int rem[]={1,2,2,3};
    auto r=castle::remove(rem,rem+4,2);
    EXPECT_EQ(r-rem,2);
    EXPECT_EQ(rem[0],1);
    EXPECT_EQ(rem[1],3);
    int rem2[]={1,2,4,3};
    auto r2=castle::remove_if(rem2,rem2+4,[](int x){return (x%2)==0;});
    EXPECT_EQ(r2-rem2,2);
    EXPECT_EQ(rem2[1],3);
    int uniq[]={1,1,2,2,3};
    auto u=castle::unique(uniq,uniq+5);
    EXPECT_EQ(u-uniq,3);
    EXPECT_EQ(uniq[0],1);
    EXPECT_EQ(uniq[2],3);
    int uniq2[]={1,1,2,4,4};
    auto u2=castle::unique(uniq2,uniq2+5,[](int x,int y){return x==y;});
    EXPECT_EQ(u2-uniq2,3);
    EXPECT_EQ(uniq2[2],4);
    EXPECT_EQ(castle::unique(uniq,uniq),uniq);
    EXPECT_EQ(castle::remove(uniq,uniq,1),uniq);
    EXPECT_EQ(castle::remove_if(uniq,uniq,[](int){return true;}),uniq);
}

TEST(AlgorithmTest, SortBoundsSearchMinMaxAndTraits)
{
    int a[]={5,1,4,2,3};
    castle::sort(a,a+5);
    for(int i=0;i<5;++i)
    {
        EXPECT_EQ(a[i],i+1);
    }
    EXPECT_EQ(castle::lower_bound(a,a+5,3)-a,2);
    EXPECT_EQ(castle::lower_bound(a,a+5,6)-a,5);
    EXPECT_EQ(castle::upper_bound(a,a+5,3)-a,3);
    EXPECT_EQ(castle::upper_bound(a,a+5,6)-a,5);
    EXPECT_TRUE(castle::binary_search(a,a+5,3));
    EXPECT_FALSE(castle::binary_search(a,a+5,9));
    castle::sort(a,a+1);
    castle::sort(a,a);
    castle::sort(a,a+5,[](int x,int y){return x>y;});
    EXPECT_EQ(a[0],5);
    EXPECT_EQ(a[4],1);
    auto comp=[](int x,int y){return x>y;};
    EXPECT_EQ(castle::lower_bound(a,a+5,3,comp)-a,2);
    EXPECT_EQ(castle::upper_bound(a,a+5,3,comp)-a,3);
    EXPECT_TRUE(castle::binary_search(a,a+5,3,comp));
    EXPECT_FALSE(castle::binary_search(a,a+5,9,comp));
    EXPECT_EQ(castle::min(2,3),2);
    EXPECT_EQ(castle::max(2,3),3);
    EXPECT_EQ(castle::min(2,3,[](int x,int y){return x<y;}),2);
    EXPECT_EQ(castle::max(2,3,[](int x,int y){return x<y;}),3);
    auto mn=castle::min_element(a,a+5);
    auto mx=castle::max_element(a,a+5);
    EXPECT_EQ(*mn,1); EXPECT_EQ(*mx,5);
    EXPECT_EQ(castle::min_element(a,a),a);
    EXPECT_EQ(castle::max_element(a,a),a);
    int desc[]={5,4,3,2,1};
    EXPECT_EQ(*castle::min_element(desc,desc+5,[](int x,int y){return x<y;}),1);
    EXPECT_EQ(*castle::max_element(desc,desc+5,[](int x,int y){return x<y;}),5);
    static_assert(castle::detail::iterator_category_is_compatible<castle::random_access_iterator_tag,castle::input_iterator_tag>::value,"compatible");
    static_assert(castle::detail::iterator_meets_category<int*,castle::random_access_iterator_tag>::value,"ptr category");
    static_assert(castle::detail::less{}(1,2),"constexpr less");
    static_assert(castle::min(1,2)==1,"constexpr min");
    static_assert(castle::max(1,2)==2,"constexpr max");
}
}
