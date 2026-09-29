#include <gtest/gtest.h>
#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"
#include "castle_ext/protocols/timing/ptp_servo.hpp"

#include <limits>

#if !defined(__ZEPHYR__) && (CASTLE_USING_POSIX_APIS || CASTLE_USING_TIMEX)
namespace
{
struct fake_clock_api
{
    inline static timespec current{};
    inline static timespec adjusted{};
    inline static int get_result = 0;
    inline static int set_result = 0;
    inline static int get_calls = 0;
    inline static int set_calls = 0;
#if CASTLE_USING_TIMEX
    inline static struct timex last_adjustment{};
    inline static int adjust_result = 0;
    inline static int adjust_calls = 0;
#endif

    static void reset()
    {
        current = timespec{};
        adjusted = timespec{};
        get_result = 0;
        set_result = 0;
        get_calls = 0;
        set_calls = 0;
#if CASTLE_USING_TIMEX
        last_adjustment = timex{};
        adjust_result = 0;
        adjust_calls = 0;
#endif
    }

    static int get_time(clockid_t, timespec* value)
    {
        ++get_calls;
        if (get_result == 0)
        {
            *value = current;
        }
        return get_result;
    }

    static int set_time(clockid_t, timespec const* value)
    {
        ++set_calls;
        adjusted = *value;
        return set_result;
    }

#if CASTLE_USING_TIMEX
    static int adjust_time(clockid_t, struct timex* adjustment)
    {
        ++adjust_calls;
        last_adjustment = *adjustment;
        return adjust_result;
    }
#endif
};
}
#endif

TEST(GccClockVariantTest, HooksAreCallable)
{
    auto realtime = castle::chrono::system_clock_adapter::realtime_ns();
    auto monotonic = castle::chrono::system_clock_adapter::monotonic_ns();
    EXPECT_GE(realtime.tv_sec, 0);
    EXPECT_GE(realtime.tv_nsec, 0);
    EXPECT_GE(monotonic.tv_sec, 0);
    EXPECT_GE(monotonic.tv_nsec, 0);
}

TEST(GccClockVariantTest, SystemClockAdapterSupportsPtpNoOpAction)
{
    castle::chrono::system_clock_adapter clock{};
    castle::timing::ptp::clock_adjustment adjustment{};

    EXPECT_EQ(
        castle::timing::ptp::apply_clock_adjustment(clock, adjustment),
        castle::status::ok);
}

#if !defined(__ZEPHYR__) && (CASTLE_USING_POSIX_APIS || CASTLE_USING_TIMEX)
TEST(GccClockVariantTest, StepReturnsSystemCallErrorWhenReadingFails)
{
    fake_clock_api::reset();
    fake_clock_api::get_result = -1;
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(1000000000LL),
        castle::status::system_call_error);
    EXPECT_EQ(fake_clock_api::get_calls, 1);
    EXPECT_EQ(fake_clock_api::set_calls, 0);
}

TEST(GccClockVariantTest, StepNormalizesPositiveNanosecondOverflow)
{
    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = 12;
    fake_clock_api::current.tv_nsec = 900000000L;
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(200000000LL),
        castle::status::ok);
    EXPECT_EQ(fake_clock_api::adjusted.tv_sec, 13);
    EXPECT_EQ(fake_clock_api::adjusted.tv_nsec, 100000000L);
}

TEST(GccClockVariantTest, StepNormalizesNegativeNanosecondUnderflow)
{
    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = 12;
    fake_clock_api::current.tv_nsec = 100000000L;
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(-200000000LL),
        castle::status::ok);
    EXPECT_EQ(fake_clock_api::adjusted.tv_sec, 11);
    EXPECT_EQ(fake_clock_api::adjusted.tv_nsec, 900000000L);
}

TEST(GccClockVariantTest, StepReturnsSystemCallErrorWhenSettingFails)
{
    fake_clock_api::reset();
    fake_clock_api::set_result = -1;
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(0),
        castle::status::system_call_error);
    EXPECT_EQ(fake_clock_api::set_calls, 1);
}

TEST(GccClockVariantTest, StepDetectsPositiveSecondsOverflow)
{
    if (std::numeric_limits<time_t>::max() < std::numeric_limits<int64_t>::max())
    {
        GTEST_SKIP() << "time_t cannot represent the int64 boundary on this target";
    }

    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = static_cast<time_t>(std::numeric_limits<int64_t>::max());
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(1000000000LL),
        castle::status::out_of_range);
    EXPECT_EQ(fake_clock_api::set_calls, 0);
}

TEST(GccClockVariantTest, StepDetectsNegativeSecondsOverflow)
{
    if (std::numeric_limits<time_t>::min() > std::numeric_limits<int64_t>::min())
    {
        GTEST_SKIP() << "time_t cannot represent the int64 boundary on this target";
    }

    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = static_cast<time_t>(std::numeric_limits<int64_t>::min());
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(-1000000000LL),
        castle::status::out_of_range);
    EXPECT_EQ(fake_clock_api::set_calls, 0);
}

TEST(GccClockVariantTest, StepDetectsPositiveNanosecondCarryAtMaximumSeconds)
{
    if (std::numeric_limits<time_t>::max() < std::numeric_limits<int64_t>::max())
    {
        GTEST_SKIP() << "time_t cannot represent the int64 boundary on this target";
    }

    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = static_cast<time_t>(std::numeric_limits<int64_t>::max());
    fake_clock_api::current.tv_nsec = 999999999L;
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(1LL),
        castle::status::out_of_range);
    EXPECT_EQ(fake_clock_api::set_calls, 0);
}

TEST(GccClockVariantTest, StepDetectsNegativeNanosecondBorrowAtMinimumSeconds)
{
    if (std::numeric_limits<time_t>::min() > std::numeric_limits<int64_t>::min())
    {
        GTEST_SKIP() << "time_t cannot represent the int64 boundary on this target";
    }

    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = static_cast<time_t>(std::numeric_limits<int64_t>::min());
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(-1LL),
        castle::status::out_of_range);
    EXPECT_EQ(fake_clock_api::set_calls, 0);
}

TEST(GccClockVariantTest, StepDetectsTimeTConversionOverflowWhenTimeTIsNarrower)
{
    if (std::numeric_limits<time_t>::max() >= std::numeric_limits<int64_t>::max())
    {
        GTEST_SKIP() << "time_t has the full int64 range on this target";
    }

    fake_clock_api::reset();
    fake_clock_api::current.tv_sec = std::numeric_limits<time_t>::max();
    castle::chrono::system_clock_adapter clock{};

    EXPECT_EQ(
        clock.step<fake_clock_api>(1000000000LL),
        castle::status::out_of_range);
    EXPECT_EQ(fake_clock_api::set_calls, 0);
}

TEST(GccClockVariantTest, NativeSyscallPolicyHandlesSafeInputs)
{
    timespec current{};
    ASSERT_EQ(castle::chrono::detail::native_clock_api::get_time(CLOCK_REALTIME, &current), 0);

    timespec invalid{};
    invalid.tv_nsec = 1000000000L;
    EXPECT_NE(castle::chrono::detail::native_clock_api::set_time(CLOCK_REALTIME, &invalid), 0);

#if CASTLE_USING_TIMEX
    struct timex adjustment{};
    adjustment.modes = ADJ_FREQUENCY;
    EXPECT_LT(
        castle::chrono::detail::native_clock_api::adjust_time(
            static_cast<clockid_t>(-1), &adjustment),
        0);
#endif
}

#if CASTLE_USING_TIMEX
TEST(GccClockVariantTest, ScalesFrequencyInPartsPerBillion)
{
    EXPECT_EQ(castle::chrono::detail::scaled_frequency(1000), 65536);
    EXPECT_EQ(castle::chrono::detail::scaled_frequency(-1000), -65536);
}

TEST(GccClockVariantTest, SlewReturnsStatusFromTimexCall)
{
    fake_clock_api::reset();
    castle::chrono::system_clock_adapter clock{};
    EXPECT_EQ(
        clock.slew<fake_clock_api>(1000),
        castle::status::ok);
    EXPECT_EQ(fake_clock_api::adjust_calls, 1);
    EXPECT_EQ(fake_clock_api::last_adjustment.modes, ADJ_FREQUENCY);
    EXPECT_EQ(fake_clock_api::last_adjustment.freq, 65536);

    fake_clock_api::adjust_result = -1;
    EXPECT_EQ(
        clock.slew<fake_clock_api>(-1000),
        castle::status::system_call_error);
    EXPECT_EQ(fake_clock_api::last_adjustment.freq, -65536);
}
#endif
#endif
