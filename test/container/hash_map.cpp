#include <gtest/gtest.h>
#include "castle/container/hash_map.h"

namespace
{
TEST(HashMapTest, AliasIsUsable)
{
    castle::container::hash_map<int,int,2> m;
    EXPECT_EQ(m.insert(1,7),castle::status::ok);
    EXPECT_TRUE(m.contains(1));
    EXPECT_EQ(*m.get(1),7);
    EXPECT_EQ(m.erase(1),castle::status::ok);
    EXPECT_TRUE(m.empty());
}
}
