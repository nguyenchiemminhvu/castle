#include <gtest/gtest.h>

#include "castle/core/config.hpp"

#if CASTLE_USING_POSIX_APIS && defined(__linux__)
    #define CASTLE_EXT_ENABLE_GPIO_LINUX_FILE
    #include <fcntl.h>
    #include <unistd.h>
#endif

#include "castle_ext/io/gpio/backends/linux_file.hpp"

#if CASTLE_USING_POSIX_APIS && defined(__linux__)

TEST(GpioLinuxFileBackend, LogicalRawAndActiveLowOperations)
{
    using namespace castle::io::gpio;
    const char* path = "/tmp/castle_gpio_value_backend_test";
    const int descriptor = ::open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    ASSERT_GE(descriptor, 0);
    ASSERT_EQ(::write(descriptor, "0\n", 2U), 2);
    ASSERT_EQ(::close(descriptor), 0);

    const linux_file::pin_handle handle(path, true);
    level value = level::low;
    EXPECT_EQ(linux_file::backend::configure(handle, pin_config()), castle::status::not_configured);
    EXPECT_EQ(linux_file::backend::read(handle, value), castle::status::ok);
    EXPECT_EQ(value, level::high);
    EXPECT_EQ(linux_file::backend::write(handle, level::low), castle::status::ok);
    EXPECT_EQ(linux_file::backend::read_raw(handle, value), castle::status::ok);
    EXPECT_EQ(value, level::high);
    EXPECT_EQ(linux_file::backend::toggle(handle), castle::status::ok);
    EXPECT_EQ(linux_file::backend::read_raw(handle, value), castle::status::ok);
    EXPECT_EQ(value, level::low);
    EXPECT_EQ(linux_file::backend::write_raw(handle, level::high), castle::status::ok);
    EXPECT_EQ(linux_file::backend::read(handle, value), castle::status::ok);
    EXPECT_EQ(value, level::low);
    EXPECT_EQ(::unlink(path), 0);
}

TEST(GpioLinuxFileBackend, InvalidAndUnavailablePaths)
{
    using namespace castle::io::gpio;
    const linux_file::pin_handle invalid(nullptr, false);
    level value = level::low;
    EXPECT_EQ(linux_file::backend::read(invalid, value), castle::status::invalid_argument);
    EXPECT_EQ(linux_file::backend::write(invalid, level::high), castle::status::invalid_argument);
    const linux_file::pin_handle missing("/castle/no/such/gpio-value", false);
    EXPECT_EQ(linux_file::backend::read(missing, value), castle::status::system_call_error);
    EXPECT_EQ(linux_file::backend::write(missing, level::high), castle::status::system_call_error);
}

#else

TEST(GpioLinuxFileBackend, FeatureDisabledOnThisTarget)
{
    SUCCEED();
}

#endif
