#include <gtest/gtest.h>

#include "castle_ext/io/fd/descriptor.hpp"
#include "castle/utility/move.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <errno.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace
{

TEST(FdDescriptor, ReportsAvailabilityConsistently)
{
    EXPECT_EQ(castle::io::fd::descriptor_available, castle::io::fd::posix_available);
}

TEST(FdDescriptor, RejectsInvalidOpenAndBufferArguments)
{
    int descriptor = 12;
    int system_error = 99;
    EXPECT_EQ(castle::io::fd::open_fd(nullptr, 0, descriptor, 0U, &system_error), castle::status::invalid_argument);
    EXPECT_EQ(descriptor, -1);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(castle::io::fd::open_fd("", 0, descriptor), castle::status::invalid_argument);

    castle::size_type transferred = 55U;
    EXPECT_EQ(castle::io::fd::read_fd(-1, nullptr, 1U, transferred), castle::status::invalid_argument);
    EXPECT_EQ(transferred, 0U);
    EXPECT_EQ(castle::io::fd::write_fd(-1, nullptr, 1U, transferred), castle::status::invalid_argument);
    EXPECT_EQ(transferred, 0U);
    EXPECT_EQ(castle::io::fd::read_fd(0, nullptr, 0U, transferred), castle::status::ok);
    EXPECT_EQ(castle::io::fd::write_fd(0, nullptr, 0U, transferred), castle::status::ok);
    EXPECT_EQ(castle::io::fd::read_exact_fd(0, nullptr, 0U, transferred), castle::status::ok);
    EXPECT_EQ(transferred, 0U);
    system_error = 55;
    EXPECT_EQ(castle::io::fd::read_exact_fd(0, nullptr, 0U, transferred, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(transferred, 0U);
    system_error = 55;
    EXPECT_EQ(castle::io::fd::write_all_fd(1, nullptr, 0U, transferred, &system_error), castle::status::ok);
    EXPECT_EQ(system_error, 0);
    EXPECT_EQ(transferred, 0U);
#if CASTLE_USING_POSIX_APIS
    if (static_cast<castle::size_type>(SSIZE_MAX) < static_cast<castle::size_type>(-1))
    {
        const castle::size_type oversized = static_cast<castle::size_type>(SSIZE_MAX) + 1U;
        char byte = 0;
        EXPECT_EQ(castle::io::fd::read_fd(0, &byte, oversized, transferred), castle::status::out_of_range);
        EXPECT_EQ(transferred, 0U);
        EXPECT_EQ(castle::io::fd::write_fd(1, &byte, oversized, transferred), castle::status::out_of_range);
        EXPECT_EQ(transferred, 0U);
    }
#endif
}

#if CASTLE_USING_POSIX_APIS

TEST(FdDescriptor, BasicFdOwnsAndTransfersDescriptor)
{
    castle::io::fd::basic_fd invalid_adoption(-2);
    EXPECT_FALSE(invalid_adoption.is_open());
    EXPECT_EQ(invalid_adoption.native_handle(), -1);

    castle::io::fd::basic_fd original;
    EXPECT_FALSE(original.is_open());
    EXPECT_EQ(original.close(), castle::status::not_configured);
    castle::size_type empty_count = 0U;
    EXPECT_EQ(original.read_some(nullptr, 0U, empty_count), castle::status::not_configured);
    EXPECT_EQ(empty_count, 0U);

    ASSERT_EQ(original.open("/dev/null", O_RDWR), castle::status::ok);
    EXPECT_TRUE(original.is_open());
    EXPECT_TRUE(castle::io::fd::is_valid_fd(original.native_handle()));
    EXPECT_EQ(original.open("/dev/null", O_RDWR), castle::status::invalid_config);
    EXPECT_EQ(original.set_nonblocking(true), castle::status::ok);
    EXPECT_EQ(original.set_nonblocking(false), castle::status::ok);
    EXPECT_EQ(original.set_close_on_exec(true), castle::status::ok);
    EXPECT_EQ(original.set_close_on_exec(false), castle::status::ok);

    const int adopted = original.release();
    EXPECT_FALSE(original.is_open());
    castle::io::fd::basic_fd moved(adopted);
    castle::io::fd::basic_fd assigned;
    assigned = castle::move(moved);
    EXPECT_FALSE(moved.is_open());
    EXPECT_TRUE(assigned.is_open());
    EXPECT_EQ(assigned.reset(assigned.native_handle()), castle::status::ok);
    EXPECT_EQ(assigned.reset(-2), castle::status::invalid_argument);
    EXPECT_EQ(assigned.close(), castle::status::ok);
}

TEST(FdDescriptor, ReadsAndWritesPipeData)
{
    int pipe_descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(pipe_descriptors), 0);
    castle::io::fd::basic_fd reader(pipe_descriptors[0]);
    castle::io::fd::basic_fd writer(pipe_descriptors[1]);

    const char tx[] = {'A', 'B', 'C'};
    castle::size_type sent = 0U;
    EXPECT_EQ(writer.write_some(tx, sizeof(tx), sent), castle::status::ok);
    EXPECT_EQ(sent, sizeof(tx));

    char rx[sizeof(tx)] = {};
    castle::size_type received = 0U;
    EXPECT_EQ(reader.read_exact(rx, sizeof(rx), received), castle::status::ok);
    EXPECT_EQ(received, sizeof(rx));
    EXPECT_EQ(rx[0], 'A');
    EXPECT_EQ(rx[1], 'B');
    EXPECT_EQ(rx[2], 'C');
    ASSERT_EQ(writer.close(), castle::status::ok);
    EXPECT_EQ(reader.read_some(rx, sizeof(rx), received), castle::status::empty);
    EXPECT_EQ(received, 0U);
}

TEST(FdDescriptor, NonBlockingEmptyAndFullStatuses)
{
    int pipe_descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(pipe_descriptors), 0);
    castle::io::fd::basic_fd reader(pipe_descriptors[0]);
    castle::io::fd::basic_fd writer(pipe_descriptors[1]);
    ASSERT_EQ(reader.set_nonblocking(true), castle::status::ok);
    ASSERT_EQ(writer.set_nonblocking(true), castle::status::ok);

    char value = 0;
    castle::size_type count = 0U;
    EXPECT_EQ(reader.read_some(&value, 1U, count), castle::status::empty);
    EXPECT_EQ(count, 0U);

    char block[4096];
    for (castle::size_type i = 0U; i < sizeof(block); ++i) { block[i] = 'x'; }
    castle::status result = castle::status::ok;
    castle::size_type total = 0U;
    while (result == castle::status::ok)
    {
        result = writer.write_some(block, sizeof(block), count);
        total += count;
    }
    EXPECT_EQ(result, castle::status::full);
    EXPECT_GT(total, 0U);
}

TEST(FdDescriptor, ExactReadReportsPartialEof)
{
    int pipe_descriptors[2] = {-1, -1};
    ASSERT_EQ(::pipe(pipe_descriptors), 0);
    castle::io::fd::basic_fd reader(pipe_descriptors[0]);
    castle::io::fd::basic_fd writer(pipe_descriptors[1]);
    const char value = 'Q';
    castle::size_type sent = 0U;
    ASSERT_EQ(writer.write_all(&value, 1U, sent), castle::status::ok);
    ASSERT_EQ(writer.close(), castle::status::ok);

    char buffer[2] = {};
    castle::size_type received = 0U;
    EXPECT_EQ(reader.read_exact(buffer, sizeof(buffer), received), castle::status::data_loss);
    EXPECT_EQ(received, 1U);
    EXPECT_EQ(buffer[0], 'Q');
}

TEST(FdDescriptor, ReadAndWriteErrorsCaptureErrno)
{
    char value = 0;
    castle::size_type count = 0U;
    int system_error = 0;
    EXPECT_EQ(castle::io::fd::read_fd(999999, &value, 1U, count, &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    system_error = 0;
    EXPECT_EQ(castle::io::fd::write_fd(999999, &value, 1U, count, &system_error), castle::status::system_call_error);
    EXPECT_NE(system_error, 0);

    int descriptor = -1;
    EXPECT_EQ(castle::io::fd::open_fd("/castle/does/not/exist", O_RDONLY, descriptor, 0U, &system_error),
              castle::status::system_call_error);
    EXPECT_EQ(descriptor, -1);
    EXPECT_NE(system_error, 0);
}

#else

TEST(FdDescriptor, PosixStubsRemainCallable)
{
    castle::io::fd::basic_fd descriptor;
    EXPECT_EQ(descriptor.open("/dev/null", 0), castle::status::not_configured);
    castle::size_type count = 0U;
    EXPECT_EQ(descriptor.read_some(nullptr, 0U, count), castle::status::not_configured);
    EXPECT_EQ(count, 0U);
}

#endif

} // namespace
