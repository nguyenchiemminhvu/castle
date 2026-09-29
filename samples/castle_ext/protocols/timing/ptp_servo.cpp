#include <stdint.h>

#include "sample_support.hpp"
#include "castle_ext/protocols/timing/ptp_servo.hpp"

namespace
{

struct fake_clock
{
    int64_t step_offset = 0LL;
    int32_t slew_ppb = 0;

    castle::status step(int64_t offset_nanoseconds) noexcept
    {
        step_offset = offset_nanoseconds;
        return castle::status::ok;
    }

    castle::status slew(int32_t frequency_ppb) noexcept
    {
        slew_ppb = frequency_ppb;
        return castle::status::ok;
    }
};

} // namespace

int main()
{
    castle::timing::ptp::servo_config config{};
    config.step_threshold_nanoseconds = 250000000LL;
    castle::timing::ptp::proportional_servo servo(config);
    castle::timing::ptp::clock_adjustment adjustment{};

    CASTLE_SAMPLE_CHECK(servo.update(300000000LL, 1000000000ULL, adjustment) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(adjustment.mode == castle::timing::ptp::adjustment_mode::step);
    CASTLE_SAMPLE_CHECK(adjustment.offset_nanoseconds == -300000000LL);

    CASTLE_SAMPLE_CHECK(servo.update(200000000LL, 1000000000ULL, adjustment) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(adjustment.mode == castle::timing::ptp::adjustment_mode::slew);
    CASTLE_SAMPLE_CHECK(adjustment.frequency_ppb == -500000);

    CASTLE_SAMPLE_CHECK(servo.update(8000LL, 1000000000ULL, adjustment) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(adjustment.mode == castle::timing::ptp::adjustment_mode::slew);
    CASTLE_SAMPLE_CHECK(adjustment.frequency_ppb == -1000);

    fake_clock clock;
    CASTLE_SAMPLE_CHECK(castle::timing::ptp::apply_clock_adjustment(clock, adjustment) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(clock.slew_ppb == -1000);

    return 0;
}
