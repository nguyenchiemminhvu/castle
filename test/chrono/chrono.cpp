#include <gtest/gtest.h>
#include "castle/chrono/chrono.h"

TEST(ChronoTest, UmbrellaCompilesAndAliases)
{
    castle::chrono::milliseconds m(10);
    castle::chrono::seconds s(1);
    EXPECT_EQ(castle::chrono::duration_cast<castle::chrono::milliseconds>(s).count(),1000);
    EXPECT_EQ((m+s).count(),1010);
}
