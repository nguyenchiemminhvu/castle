#include <gtest/gtest.h>

#include "castle_ext/io/fd/control.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace
{

TEST(FdControl, PosixAvailabilityMatchesBuildConfiguration)
{
#if CASTLE_USING_POSIX_APIS
    EXPECT_TRUE(castle::io::fd::posix_available);
#else
    EXPECT_FALSE(castle::io::fd::posix_available);
#endif
}

TEST(FdControl, RejectsNegativeDescriptorsAndClearsOutputs)
{
    int value = 123;
    int system_error = 999;

    EXPECT_EQ(castle::io::fd::get_status_flags(-1, value, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(value, 0);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(castle::io::fd::set_status_flags(-1, 0, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(castle::io::fd::get_descriptor_flags(-1, value, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(value, 0);
    EXPECT_EQ(castle::io::fd::set_descriptor_flags(-1, 0, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(castle::io::fd::set_nonblocking(-1, true, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(castle::io::fd::set_blocking(-1, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(castle::io::fd::set_close_on_exec(-1, true, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(castle::io::fd::close_fd(-1, &system_error), castle::status::invalid_argument);
    EXPECT_FALSE(castle::io::fd::is_valid_fd(-1));
}

#if CASTLE_USING_POSIX_APIS
TEST(FdControl, StatusFlagsAndNonblockingRoundTrip)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);

    int flags = 0;
    ASSERT_EQ(castle::io::fd::get_status_flags(descriptors[0], flags), castle::status::ok);
    const int original_flags = flags;

    EXPECT_EQ(castle::io::fd::set_nonblocking(descriptors[0], true), castle::status::ok);
    ASSERT_EQ(castle::io::fd::get_status_flags(descriptors[0], flags), castle::status::ok);
    EXPECT_NE(flags & O_NONBLOCK, 0);

    EXPECT_EQ(castle::io::fd::set_nonblocking(descriptors[0], true), castle::status::ok);
    EXPECT_EQ(castle::io::fd::set_blocking(descriptors[0]), castle::status::ok);
    ASSERT_EQ(castle::io::fd::get_status_flags(descriptors[0], flags), castle::status::ok);
    EXPECT_EQ(flags & O_NONBLOCK, 0);

    // Exercise direct setters while restoring the original flags before close.
    EXPECT_EQ(castle::io::fd::set_status_flags(descriptors[0], original_flags | O_NONBLOCK), castle::status::ok);
    ASSERT_EQ(castle::io::fd::get_status_flags(descriptors[0], flags), castle::status::ok);
    EXPECT_NE(flags & O_NONBLOCK, 0);
    EXPECT_EQ(castle::io::fd::set_status_flags(descriptors[0], original_flags), castle::status::ok);

    EXPECT_EQ(castle::io::fd::close_fd(descriptors[0]), castle::status::ok);
    EXPECT_EQ(castle::io::fd::close_fd(descriptors[1]), castle::status::ok);
}

TEST(FdControl, DescriptorFlagsAndCloseOnExecRoundTrip)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);

    int flags = 0;
    ASSERT_EQ(castle::io::fd::get_descriptor_flags(descriptors[0], flags), castle::status::ok);
    const int original_flags = flags;
    EXPECT_EQ(castle::io::fd::set_close_on_exec(descriptors[0], true), castle::status::ok);
    ASSERT_EQ(castle::io::fd::get_descriptor_flags(descriptors[0], flags), castle::status::ok);
    EXPECT_NE(flags & FD_CLOEXEC, 0);
    EXPECT_EQ(castle::io::fd::set_close_on_exec(descriptors[0], true), castle::status::ok);
    EXPECT_EQ(castle::io::fd::set_close_on_exec(descriptors[0], false), castle::status::ok);
    ASSERT_EQ(castle::io::fd::get_descriptor_flags(descriptors[0], flags), castle::status::ok);
    EXPECT_EQ(flags & FD_CLOEXEC, 0);

    EXPECT_EQ(castle::io::fd::set_descriptor_flags(descriptors[0], original_flags | FD_CLOEXEC), castle::status::ok);
    ASSERT_EQ(castle::io::fd::get_descriptor_flags(descriptors[0], flags), castle::status::ok);
    EXPECT_NE(flags & FD_CLOEXEC, 0);
    EXPECT_EQ(castle::io::fd::set_descriptor_flags(descriptors[0], original_flags), castle::status::ok);

    EXPECT_TRUE(castle::io::fd::is_valid_fd(descriptors[0]));
    EXPECT_EQ(castle::io::fd::close_fd(descriptors[0]), castle::status::ok);
    EXPECT_FALSE(castle::io::fd::is_valid_fd(descriptors[0]));
    EXPECT_EQ(castle::io::fd::close_fd(descriptors[1]), castle::status::ok);
}

TEST(FdControl, SystemCallFailureReportsErrno)
{
    int descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(descriptors), 0);
    const int closed_fd = descriptors[0];
    ASSERT_EQ(castle::io::fd::close_fd(closed_fd), castle::status::ok);

    int flags = 123;
    int system_error = 0;
    EXPECT_EQ(castle::io::fd::get_status_flags(closed_fd, flags, &system_error), castle::status::system_call_error);
    EXPECT_EQ(flags, 0);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(castle::io::fd::set_status_flags(closed_fd, 0, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(castle::io::fd::get_descriptor_flags(closed_fd, flags, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(castle::io::fd::set_descriptor_flags(closed_fd, 0, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(castle::io::fd::set_nonblocking(closed_fd, true, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(castle::io::fd::set_close_on_exec(closed_fd, true, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_FALSE(castle::io::fd::is_valid_fd(closed_fd));
    EXPECT_EQ(castle::io::fd::close_fd(closed_fd, &system_error), castle::status::system_call_error);
    EXPECT_EQ(system_error, EBADF);
    EXPECT_EQ(castle::io::fd::close_fd(descriptors[1]), castle::status::ok);
}
#else
TEST(FdControl, NonPosixCallsReportNotConfigured)
{
    int flags = 0;
    EXPECT_EQ(castle::io::fd::get_status_flags(4, flags), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::set_status_flags(4, 0), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::get_descriptor_flags(4, flags), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::set_descriptor_flags(4, 0), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::set_nonblocking(4), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::set_blocking(4), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::set_close_on_exec(4), castle::status::not_configured);
    EXPECT_EQ(castle::io::fd::close_fd(4), castle::status::not_configured);
    EXPECT_FALSE(castle::io::fd::is_valid_fd(4));
}
#endif

} // namespace
