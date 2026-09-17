#include "sample_support.hpp"

#include "castle/chrono/duration.hpp"

#include <stdint.h>

int main()
{
    using half_seconds = castle::chrono::duration<int64_t, castle::math::ratio<1, 2> >;
    using milliseconds32 = castle::chrono::duration<int32_t, castle::milli>;

    CASTLE_SAMPLE_CHECK((castle::chrono::is_valid_period<castle::milli>::value));
    CASTLE_SAMPLE_CHECK(castle::chrono::duration_values<int32_t>::zero() == 0);

    castle::chrono::milliseconds sample_period(10);
    castle::chrono::milliseconds copied(sample_period);
    milliseconds32 narrow(sample_period);
    castle::chrono::milliseconds from_seconds(castle::chrono::seconds(2));
    castle::chrono::microseconds finer = castle::chrono::duration_cast<castle::chrono::microseconds>(sample_period);

    CASTLE_SAMPLE_CHECK(sample_period.count() == 10);
    CASTLE_SAMPLE_CHECK(copied.count() == 10);
    CASTLE_SAMPLE_CHECK(narrow.count() == 10);
    CASTLE_SAMPLE_CHECK(from_seconds.count() == 2000);
    CASTLE_SAMPLE_CHECK(finer.count() == 10000);
    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds::zero().count() == 0);
    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds::min() < castle::chrono::milliseconds::zero());
    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds::max() > castle::chrono::milliseconds::zero());

    ++sample_period;
    sample_period++;
    --sample_period;
    sample_period--;
    CASTLE_SAMPLE_CHECK(sample_period.count() == 10);

    sample_period += castle::chrono::milliseconds(5);
    sample_period -= castle::chrono::milliseconds(2);
    sample_period *= 3;
    sample_period /= 2;
    sample_period %= 4;
    sample_period %= castle::chrono::milliseconds(3);
    CASTLE_SAMPLE_CHECK(sample_period.count() == 0);

    const auto positive = +castle::chrono::milliseconds(7);
    const auto negative = -castle::chrono::milliseconds(7);
    const auto total = castle::chrono::seconds(1) + castle::chrono::milliseconds(500);
    const auto difference = castle::chrono::seconds(2) - castle::chrono::milliseconds(500);
    const auto scaled_left = castle::chrono::milliseconds(7) * 3;
    const auto scaled_right = 2 * castle::chrono::milliseconds(7);
    const auto divided = castle::chrono::milliseconds(9) / 2;
    const auto ratio = castle::chrono::operator/<
        int64_t, castle::seconds, int64_t, castle::milli>(
            castle::chrono::seconds(3),
            castle::chrono::milliseconds(500));
    const auto mod_scalar = castle::chrono::milliseconds(10) % 6;
    const auto mod_duration = castle::chrono::operator%<
        int64_t, castle::seconds, int64_t, castle::milli>(
            castle::chrono::seconds(5),
            castle::chrono::milliseconds(1200));

    CASTLE_SAMPLE_CHECK(positive.count() == 7);
    CASTLE_SAMPLE_CHECK(negative.count() == -7);
    CASTLE_SAMPLE_CHECK(total.count() == 1500);
    CASTLE_SAMPLE_CHECK(difference.count() == 1500);
    CASTLE_SAMPLE_CHECK(scaled_left.count() == 21);
    CASTLE_SAMPLE_CHECK(scaled_right.count() == 14);
    CASTLE_SAMPLE_CHECK(divided.count() == 4);
    CASTLE_SAMPLE_CHECK(ratio == 6);
    CASTLE_SAMPLE_CHECK(mod_scalar.count() == 4);
    CASTLE_SAMPLE_CHECK(mod_duration.count() == 200);

    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds(1000) == castle::chrono::seconds(1));
    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds(1000) != castle::chrono::milliseconds(999));
    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds(999) < castle::chrono::seconds(1));
    CASTLE_SAMPLE_CHECK(castle::chrono::milliseconds(1000) <= castle::chrono::seconds(1));
    CASTLE_SAMPLE_CHECK(castle::chrono::seconds(2) > castle::chrono::milliseconds(1500));
    CASTLE_SAMPLE_CHECK(castle::chrono::seconds(2) >= castle::chrono::milliseconds(2000));

    const half_seconds three_half(3);
    const half_seconds five_half(5);
    CASTLE_SAMPLE_CHECK(castle::chrono::floor<castle::chrono::seconds>(three_half).count() == 1);
    CASTLE_SAMPLE_CHECK(castle::chrono::ceil<castle::chrono::seconds>(three_half).count() == 2);
    CASTLE_SAMPLE_CHECK(castle::chrono::round<castle::chrono::seconds>(three_half).count() == 2);
    CASTLE_SAMPLE_CHECK(castle::chrono::round<castle::chrono::seconds>(five_half).count() == 2);
    CASTLE_SAMPLE_CHECK(castle::chrono::abs(castle::chrono::milliseconds(-7)).count() == 7);
    return 0;
}
