#include <gtest/gtest.h>
#include "castle/error/status.h"

TEST(StatusTest, ValuesAndSucceeded)
{
    static_assert(static_cast<unsigned>(castle::status::ok)==0U,"ok");

    EXPECT_TRUE(castle::succeeded(castle::status::ok));
    EXPECT_FALSE(castle::succeeded(castle::status::full));
    EXPECT_NE(castle::status::invalid_argument,castle::status::invalid_callback);
    EXPECT_NE(castle::status::already_exists,castle::status::unknown_error);
}
