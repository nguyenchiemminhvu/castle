#include <gtest/gtest.h>

#include "castle_ext/io/file/linux_file.hpp"

#if CASTLE_USING_POSIX_APIS
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace
{

using castle::io::file::file_config;
using castle::io::file::seek_origin;

TEST(LinuxFile, ValidatesConfiguration)
{
    EXPECT_EQ(castle::io::file::validate_config(file_config()), castle::status::invalid_argument);
    EXPECT_EQ(castle::io::file::validate_config(file_config("/tmp/x", false, false)), castle::status::invalid_argument);
    EXPECT_EQ(castle::io::file::validate_config(file_config("/tmp/x", true, false, false, true)), castle::status::invalid_config);
    EXPECT_EQ(castle::io::file::validate_config(file_config("/tmp/x", true, false, false, false, true)), castle::status::invalid_config);
    EXPECT_EQ(castle::io::file::validate_config(file_config("/tmp/x", true, false, false, false, false, true)), castle::status::invalid_config);
    EXPECT_EQ(castle::io::file::validate_config(file_config("/tmp/x", true, true, true, true, false, true)), castle::status::ok);
}

#if CASTLE_USING_POSIX_APIS

TEST(LinuxFile, OpensReadsWritesSeeksAndTruncates)
{
    const char* path = "/tmp/castle_linux_file_test.bin";
    (void)::unlink(path);
    castle::io::file::basic_file file;
    castle::size_type unopened_count = 99U;
    int64_t unopened_position = 0;
    const char unopened_byte = 'x';
    EXPECT_EQ(file.write_some(&unopened_byte, 1U, unopened_count), castle::status::not_configured);
    EXPECT_EQ(unopened_count, 0U);
    EXPECT_EQ(file.read_some(nullptr, 1U, unopened_count), castle::status::not_configured);
    EXPECT_EQ(unopened_count, 0U);
    EXPECT_EQ(file.truncate(0), castle::status::not_configured);
    EXPECT_EQ(file.tell(unopened_position), castle::status::not_configured);
    EXPECT_EQ(file.size(unopened_position), castle::status::not_configured);
    file_config cfg(path, true, true, true, true);
    ASSERT_EQ(file.open(cfg), castle::status::ok);
    EXPECT_TRUE(file.is_open());
    EXPECT_GE(file.native_handle(), 0);
    EXPECT_EQ(file.open(cfg), castle::status::invalid_config);

    const unsigned char tx[] = {1U, 2U, 3U, 4U, 5U};
    castle::size_type count = 0U;
    ASSERT_EQ(file.write_all(tx, sizeof(tx), count), castle::status::ok);
    EXPECT_EQ(count, sizeof(tx));

    int64_t size = -1;
    ASSERT_EQ(file.size(size), castle::status::ok);
    EXPECT_EQ(size, static_cast<int64_t>(sizeof(tx)));
    int64_t position = -1;
    ASSERT_EQ(file.tell(position), castle::status::ok);
    EXPECT_EQ(position, static_cast<int64_t>(sizeof(tx)));
    ASSERT_EQ(file.seek(0, seek_origin::begin, position), castle::status::ok);
    EXPECT_EQ(position, 0);

    unsigned char rx[sizeof(tx)] = {};
    ASSERT_EQ(file.read_exact(rx, sizeof(rx), count), castle::status::ok);
    EXPECT_EQ(count, sizeof(rx));
    EXPECT_EQ(rx[0], 1U);
    EXPECT_EQ(rx[4], 5U);
    EXPECT_EQ(file.seek(0, static_cast<seek_origin>(99U), position), castle::status::invalid_argument);
    EXPECT_EQ(file.truncate(-1), castle::status::invalid_argument);
    EXPECT_EQ(file.truncate(2), castle::status::ok);
    ASSERT_EQ(file.size(size), castle::status::ok);
    EXPECT_EQ(size, 2);
    EXPECT_EQ(file.sync(), castle::status::ok);
    EXPECT_EQ(file.close(), castle::status::ok);
    EXPECT_FALSE(file.is_open());
    EXPECT_EQ(file.tell(position), castle::status::not_configured);
    EXPECT_EQ(file.close(), castle::status::not_configured);
    (void)::unlink(path);
}

TEST(LinuxFile, ReadOnlyAndWriteOnlyRestrictions)
{
    const char* path = "/tmp/castle_linux_file_modes_test.bin";
    (void)::unlink(path);
    castle::io::file::basic_file writer;
    ASSERT_EQ(writer.open(file_config(path, false, true, true, true)), castle::status::ok);
    castle::size_type count = 77U;
    const char value = 'a';
    EXPECT_EQ(writer.read_some(nullptr, 1U, count), castle::status::invalid_config);
    EXPECT_EQ(count, 0U);
    EXPECT_EQ(writer.write_some(&value, 1U, count), castle::status::ok);
    EXPECT_EQ(count, 1U);
    EXPECT_EQ(writer.close(), castle::status::ok);

    castle::io::file::basic_file reader;
    ASSERT_EQ(reader.open(file_config(path, true, false)), castle::status::ok);
    count = 77U;
    EXPECT_EQ(reader.write_all(&value, 1U, count), castle::status::invalid_config);
    EXPECT_EQ(count, 0U);
    char received = 0;
    EXPECT_EQ(reader.read_some(&received, 1U, count), castle::status::ok);
    EXPECT_EQ(received, value);
    EXPECT_EQ(reader.truncate(0), castle::status::invalid_config);
    EXPECT_EQ(reader.close(), castle::status::ok);
    (void)::unlink(path);
}

TEST(LinuxFile, ExclusiveCreateAndSystemErrors)
{
    const char* path = "/tmp/castle_linux_file_exclusive_test.bin";
    (void)::unlink(path);
    castle::io::file::basic_file first;
    ASSERT_EQ(first.open(file_config(path, true, true, true, false, false, true)), castle::status::ok);
    castle::io::file::basic_file second;
    int system_error = 0;
    EXPECT_EQ(second.open(file_config(path, true, true, true, false, false, true), &system_error),
              castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
    EXPECT_EQ(first.close(), castle::status::ok);
    (void)::unlink(path);

    castle::io::file::basic_file missing;
    EXPECT_EQ(missing.open(file_config("/castle/missing/file", true, false), &system_error),
              castle::status::system_call_error);
    EXPECT_NE(system_error, 0);
}

#else

TEST(LinuxFile, PosixDisabledReturnsNotConfigured)
{
    castle::io::file::basic_file file;
    EXPECT_EQ(file.open(file_config("/tmp/file")), castle::status::not_configured);
}

#endif

} // namespace
