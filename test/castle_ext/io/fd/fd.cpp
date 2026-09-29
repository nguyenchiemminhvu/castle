#include <gtest/gtest.h>

#include "castle_ext/io/fd/fd.hpp"

TEST(FdUmbrella, ExposesOnePosixAvailabilityDecision)
{
    EXPECT_EQ(castle::io::fd::posix_available, castle::io::fd::poll_available);
#if CASTLE_USING_POSIX_APIS
    EXPECT_TRUE(castle::io::fd::poll_available);
    castle::io::fd::fd_event_manager manager;
    EXPECT_EQ(manager.get_fd_count(), 0U);
    EXPECT_EQ(manager.get_enabled_fd_count(), 0U);
#else
    EXPECT_FALSE(castle::io::fd::posix_available);
    EXPECT_EQ(castle::io::fd::set_nonblocking(7), castle::status::not_configured);
#endif
}
