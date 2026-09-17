#include "sample_support.hpp"

#include "castle/chrono/time_point.hpp"

#include <stdint.h>

namespace
{
struct test_clock
{
    using duration = castle::chrono::milliseconds;
};
} // namespace

int main()
{
    using point = castle::chrono::time_point<test_clock, castle::chrono::milliseconds>;
    using coarse_point = castle::chrono::time_point<test_clock, castle::chrono::seconds>;
    using half_seconds = castle::chrono::duration<int64_t, castle::math::ratio<1, 2> >;
    using fractional_point = castle::chrono::time_point<test_clock, half_seconds>;

    point start(castle::chrono::milliseconds(100));
    point epoch;
    coarse_point coarse(castle::chrono::seconds(2));
    point copied(start);
    point converted(coarse);

    CASTLE_SAMPLE_CHECK(epoch.time_since_epoch().count() == 0);
    CASTLE_SAMPLE_CHECK(copied.time_since_epoch().count() == 100);
    CASTLE_SAMPLE_CHECK(converted.time_since_epoch().count() == 2000);
    CASTLE_SAMPLE_CHECK(point::min() < point::max());

    start += castle::chrono::milliseconds(50);
    start -= castle::chrono::milliseconds(20);
    ++start;
    start++;
    --start;
    start--;
    CASTLE_SAMPLE_CHECK(start.time_since_epoch().count() == 130);

    const point deadline = start + castle::chrono::milliseconds(25);
    const point deadline_from_left = castle::chrono::milliseconds(25) + start;
    const point previous = deadline - castle::chrono::milliseconds(10);
    const auto elapsed = deadline - start;

    CASTLE_SAMPLE_CHECK(deadline.time_since_epoch().count() == 155);
    CASTLE_SAMPLE_CHECK(deadline_from_left.time_since_epoch().count() == 155);
    CASTLE_SAMPLE_CHECK(previous.time_since_epoch().count() == 145);
    CASTLE_SAMPLE_CHECK(elapsed.count() == 25);

    const coarse_point casted = castle::chrono::time_point_cast<castle::chrono::seconds>(
        point(castle::chrono::milliseconds(2500)));
    const coarse_point floored = castle::chrono::floor<castle::chrono::seconds>(
        point(castle::chrono::milliseconds(2500)));
    const coarse_point ceiled = castle::chrono::ceil<castle::chrono::seconds>(
        point(castle::chrono::milliseconds(2500)));
    const coarse_point rounded = castle::chrono::round<castle::chrono::seconds>(
        fractional_point(half_seconds(5)));

    CASTLE_SAMPLE_CHECK(casted.time_since_epoch().count() == 2);
    CASTLE_SAMPLE_CHECK(floored.time_since_epoch().count() == 2);
    CASTLE_SAMPLE_CHECK(ceiled.time_since_epoch().count() == 3);
    CASTLE_SAMPLE_CHECK(rounded.time_since_epoch().count() == 2);

    CASTLE_SAMPLE_CHECK(deadline == point(castle::chrono::milliseconds(155)));
    CASTLE_SAMPLE_CHECK(deadline != previous);
    CASTLE_SAMPLE_CHECK(previous < deadline);
    CASTLE_SAMPLE_CHECK(previous <= deadline);
    CASTLE_SAMPLE_CHECK(deadline > previous);
    CASTLE_SAMPLE_CHECK(deadline >= previous);
    return 0;
}
