#include <gtest/gtest.h>
#include "castle/container/string_view.h"

namespace
{
TEST(StringViewTest, AccessSubstrFindAndCompare)
{
    using V=castle::container::string_view;
    V empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.size(),0U);
    EXPECT_EQ(empty.end(),empty.data());
    EXPECT_EQ(empty.find('x'),V::npos);
    EXPECT_EQ(empty.find(V("")),0U);
    EXPECT_EQ(empty.find(V("x")),V::npos);
    V v("hello");
    EXPECT_EQ(v.size(),5U);
    EXPECT_EQ(v.length(),5U);
    EXPECT_EQ(v[1],'e');
    EXPECT_EQ(v.front(),'h');
    EXPECT_EQ(v.back(),'o');
    EXPECT_EQ(*v.begin(),'h');
    EXPECT_EQ(v.end(),v.data()+5);
    EXPECT_EQ(v.substr(1,3),V("ell"));
    EXPECT_EQ(v.substr(1),V("ello"));
    EXPECT_TRUE(v.substr(5).empty());
    EXPECT_EQ(v.substr(6).data(),nullptr);
    EXPECT_EQ(v.substr(2,99),V("llo"));
    EXPECT_EQ(v.find('l'),2U);
    EXPECT_EQ(v.find('l',3),3U);
    EXPECT_EQ(v.find('z'),V::npos);
    EXPECT_EQ(v.find(V("ell")),1U);
    EXPECT_EQ(v.find(V("ell"),2),V::npos);
    EXPECT_EQ(v.find(V(""),4),4U);
    EXPECT_EQ(v.find(V(""),6),V::npos);
    EXPECT_EQ(v.compare(V("hello")),0);
    EXPECT_LT(v.compare(V("hello!")),0);
    EXPECT_GT(v.compare(V("hell")),0);
    EXPECT_LT(V("abc").compare(V("abd")),0);
    EXPECT_GT(V("abe").compare(V("abd")),0);
    EXPECT_TRUE(v.starts_with(V("he")));
    EXPECT_FALSE(v.starts_with(V("eh")));
    EXPECT_TRUE(v.starts_with(V("")));
    EXPECT_TRUE(v.ends_with(V("lo")));
    EXPECT_FALSE(v.ends_with(V("el")));
    EXPECT_TRUE(v.ends_with(V("")));
    EXPECT_TRUE(v==V("hello"));
    EXPECT_TRUE(v!=V("world"));
    EXPECT_LT(V("abc"),V("abd"));
    EXPECT_GT(V("abd"),V("abc"));
    EXPECT_LE(V("abc"),V("abc"));
    EXPECT_GE(V("abc"),V("abc"));
    const char* null_text=nullptr; V nv(null_text);
    EXPECT_TRUE(nv.empty());
}
}
