#include <gtest/gtest.h>
#include "castle/core/status.hpp"

TEST(StatusTest, ValuesAndSucceeded)
{
    static_assert(static_cast<unsigned>(castle::status::ok)==0U,"ok");

    EXPECT_TRUE(castle::succeeded(castle::status::ok));
    EXPECT_FALSE(castle::succeeded(castle::status::full));
    EXPECT_FALSE(castle::succeeded(castle::status::in_progress));
    EXPECT_NE(castle::status::invalid_argument,castle::status::parse_error);
    EXPECT_NE(castle::status::not_converged,castle::status::insufficient_data);
    EXPECT_NE(castle::status::invalid_state,castle::status::unsupported);
    EXPECT_NE(castle::status::already_exists,castle::status::unknown_error);
}
