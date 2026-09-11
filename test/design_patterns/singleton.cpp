#include <gtest/gtest.h>
#include "castle/design_patterns/singleton.h"

namespace
{
    struct S
    {
        static int constructed;
        int value;
        explicit S(int v):value(v)
        {
            ++constructed;
        }
        ~S()
        {
            --constructed;
        }
    };

    int S::constructed=0;

    TEST(SingletonTest, LifecycleAndState)
    {
        using SS=castle::design_patterns::singleton<S>;
        static_assert(noexcept(SS::instance()),"instance noexcept");
        EXPECT_FALSE(SS::is_valid());
        SS::create(42);
        EXPECT_TRUE(SS::is_valid());
        EXPECT_EQ(SS::instance().value,42);
        EXPECT_EQ(S::constructed,1);
        SS::destroy();
        EXPECT_FALSE(SS::is_valid());
        EXPECT_EQ(S::constructed,0);
        SS::create(7);
        EXPECT_EQ(SS::instance().value,7);
        SS::destroy();
    }
}
