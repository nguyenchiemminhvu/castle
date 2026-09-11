#include <gtest/gtest.h>
#include "castle/container/string.h"

namespace
{
using S=castle::container::basic_string<char,8>;

TEST(StringTest, AssignAppendInsertEraseResize)
{
    S s;
    EXPECT_EQ(S::static_capacity,8U);
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.c_str()[0],'\0');
    EXPECT_EQ(s.assign(static_cast<const char*>(nullptr)),castle::status::invalid_argument);
    EXPECT_EQ(s.assign("hello"),castle::status::ok);
    EXPECT_EQ(s.size(),5U);
    EXPECT_STREQ(s.c_str(),"hello");
    EXPECT_EQ(s.front(),'h');
    EXPECT_EQ(s.back(),'o');
    s[0]='H'; EXPECT_EQ(s[0],'H');
    EXPECT_EQ(s.push_back('!'),castle::status::ok);
    EXPECT_EQ(s.pop_back(),castle::status::ok);
    EXPECT_EQ(s.append("!") ,castle::status::ok);
    EXPECT_EQ(s.append(S::view_type("?")),castle::status::ok);
    EXPECT_STREQ(s.c_str(),"Hello!?");
}

TEST(StringTest, CapacityComparisonFindAndOtherOverloads)
{
    S s("abc");
    S t(s);
    EXPECT_TRUE(t==s);
    t=s;
    EXPECT_TRUE(t==s);
    EXPECT_TRUE(s==S::view_type("abc"));
    EXPECT_TRUE(S::view_type("abc")==s);
    EXPECT_TRUE(s!=S::view_type("abd"));
    EXPECT_TRUE(S::view_type("abd")!=s);
    EXPECT_FALSE(s<S("abb"));
    EXPECT_TRUE(S("abb")<s);
    EXPECT_TRUE(s>S("abb"));
    EXPECT_TRUE(s>=t);
    EXPECT_TRUE(s<=t);
    EXPECT_EQ(s.append(static_cast<const char*>(nullptr)),castle::status::invalid_argument);
    EXPECT_EQ(s.assign(S::view_type("abcdefghi")),castle::status::full);
    EXPECT_EQ(s.append("0123456789"),castle::status::full);
    EXPECT_EQ(s.insert(99,S::view_type("x")),castle::status::out_of_range);
    EXPECT_EQ(s.insert(1,S::view_type("ZZ")),castle::status::ok);
    EXPECT_STREQ(s.c_str(),"aZZbc");
    EXPECT_EQ(s.erase(2,99),castle::status::ok);
    EXPECT_STREQ(s.c_str(),"aZ");
    EXPECT_EQ(s.resize(5,'q'),castle::status::ok);
    EXPECT_STREQ(s.c_str(),"aZqqq");
    EXPECT_EQ(s.resize(9),castle::status::full);
    EXPECT_EQ(s.find('Z'),1U);
    EXPECT_EQ(s.find('x'),S::view_type::npos);
    EXPECT_EQ(s.find(S::view_type("Z")),1U);
    EXPECT_EQ(s.compare(S::view_type("aZqqq")),0);
    EXPECT_EQ(s.view(),S::view_type("aZqqq"));
    s.clear();
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.pop_back(),castle::status::empty);
}
}
