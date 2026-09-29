#include <gtest/gtest.h>

#include "castle_ext/protocols/timing/ptp_servo.hpp"

namespace
{

struct fake_clock
{
    int64_t step_offset = 0LL;
    int32_t slew_ppb = 0;
    bool step_called = false;
    bool slew_called = false;

    castle::status step(int64_t offset_nanoseconds) noexcept
    {
        step_called = true;
        step_offset = offset_nanoseconds;
        return castle::status::ok;
    }

    castle::status slew(int32_t frequency_ppb) noexcept
    {
        slew_called = true;
        slew_ppb = frequency_ppb;
        return castle::status::ok;
    }
};

} // namespace

TEST(PtpServo, SelectsStepForLargePositiveAndNegativeOffsets)
{
    castle::timing::ptp::proportional_servo servo;
    castle::timing::ptp::clock_adjustment adjustment{};

    ASSERT_EQ(servo.update(200000000LL, 1000000000ULL, adjustment), castle::status::ok);
    EXPECT_EQ(adjustment.mode, castle::timing::ptp::adjustment_mode::step);
    EXPECT_EQ(adjustment.offset_nanoseconds, -200000000LL);

    ASSERT_EQ(servo.update(-200000000LL, 1000000000ULL, adjustment), castle::status::ok);
    EXPECT_EQ(adjustment.mode, castle::timing::ptp::adjustment_mode::step);
    EXPECT_EQ(adjustment.offset_nanoseconds, 200000000LL);
}

TEST(PtpServo, SelectsProportionalSlewAndClamps)
{
    castle::timing::ptp::proportional_servo servo;
    castle::timing::ptp::clock_adjustment adjustment{};

    ASSERT_EQ(servo.update(8000LL, 1000000000ULL, adjustment), castle::status::ok);
    EXPECT_EQ(adjustment.mode, castle::timing::ptp::adjustment_mode::slew);
    EXPECT_EQ(adjustment.frequency_ppb, -1000);

    castle::timing::ptp::servo_config config{};
    config.step_threshold_nanoseconds = 1000000000LL;
    config.max_slew_ppb = 100;
    servo.configure(config);
    ASSERT_EQ(servo.update(1000000000LL - 1LL, 1000ULL, adjustment), castle::status::ok);
    EXPECT_EQ(adjustment.mode, castle::timing::ptp::adjustment_mode::slew);
    EXPECT_EQ(adjustment.frequency_ppb, -100);
}

TEST(PtpServo, HandlesNoCorrectionAndInvalidConfiguration)
{
    castle::timing::ptp::proportional_servo servo;
    castle::timing::ptp::clock_adjustment adjustment{};

    ASSERT_EQ(servo.update(0LL, 1U, adjustment), castle::status::ok);
    EXPECT_EQ(adjustment.mode, castle::timing::ptp::adjustment_mode::none);

    castle::timing::ptp::servo_config config = servo.config();
    config.gain_denominator = 0U;
    servo.configure(config);
    EXPECT_EQ(servo.update(1LL, 1000U, adjustment), castle::status::invalid_argument);

    config = servo.config();
    config.gain_denominator = 1U;
    config.max_slew_ppb = -1;
    servo.configure(config);
    EXPECT_EQ(servo.update(1LL, 1000U, adjustment), castle::status::invalid_argument);

    config = servo.config();
    config.max_slew_ppb = 100;
    config.step_threshold_nanoseconds = -1LL;
    servo.configure(config);
    EXPECT_EQ(servo.update(1LL, 1000U, adjustment), castle::status::invalid_argument);

    config = servo.config();
    config.step_threshold_nanoseconds = 100;
    servo.configure(config);
    EXPECT_EQ(servo.update(1LL, 0U, adjustment), castle::status::invalid_argument);
}

TEST(PtpServo, AppliesClockActionsWithoutVirtualDispatch)
{
    fake_clock clock;
    castle::timing::ptp::clock_adjustment adjustment{};

    adjustment.mode = castle::timing::ptp::adjustment_mode::step;
    adjustment.offset_nanoseconds = -42LL;
    EXPECT_EQ(
        castle::timing::ptp::apply_clock_adjustment(clock, adjustment),
        castle::status::ok);
    EXPECT_TRUE(clock.step_called);
    EXPECT_EQ(clock.step_offset, -42LL);

    adjustment.mode = castle::timing::ptp::adjustment_mode::slew;
    adjustment.frequency_ppb = 1234;
    EXPECT_EQ(
        castle::timing::ptp::apply_clock_adjustment(clock, adjustment),
        castle::status::ok);
    EXPECT_TRUE(clock.slew_called);
    EXPECT_EQ(clock.slew_ppb, 1234);

    adjustment.mode = castle::timing::ptp::adjustment_mode::none;
    EXPECT_EQ(
        castle::timing::ptp::apply_clock_adjustment(clock, adjustment),
        castle::status::ok);

    adjustment.mode = static_cast<castle::timing::ptp::adjustment_mode>(99U);
    EXPECT_EQ(
        castle::timing::ptp::apply_clock_adjustment(clock, adjustment),
        castle::status::invalid_argument);
}

TEST(PtpServo, SaturatesMinimumInt64StepOffset)
{
    castle::timing::ptp::proportional_servo servo;
    castle::timing::ptp::servo_config config{};
    config.step_threshold_nanoseconds = 0LL;
    servo.configure(config);

    castle::timing::ptp::clock_adjustment adjustment{};
    ASSERT_EQ(servo.update(INT64_MIN, 1000U, adjustment), castle::status::ok);
    EXPECT_EQ(adjustment.mode, castle::timing::ptp::adjustment_mode::step);
    EXPECT_EQ(adjustment.offset_nanoseconds, INT64_MAX);
}
