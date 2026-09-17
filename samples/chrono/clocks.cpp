#include "sample_support.hpp"

#include "castle/chrono/clocks.hpp"

int main()
{
    const castle::chrono::system_clock_duration wall_ticks(1);
    const castle::chrono::steady_clock_duration steady_ticks(1);
    const auto wall_now = castle::chrono::system_clock::now();
    const auto steady_before = castle::chrono::steady_clock::now();
    const auto steady_after = castle::chrono::steady_clock::now();
    const auto converted = castle::chrono::system_clock::from_time_t(2);

    CASTLE_SAMPLE_CHECK(wall_ticks.count() == 1);
    CASTLE_SAMPLE_CHECK(steady_ticks.count() == 1);
    CASTLE_SAMPLE_CHECK(!castle::chrono::system_clock::is_steady);
    CASTLE_SAMPLE_CHECK(castle::chrono::steady_clock::is_steady);
    CASTLE_SAMPLE_CHECK((castle::is_same<
        castle::chrono::high_resolution_clock,
        castle::chrono::system_clock>::value));
    CASTLE_SAMPLE_CHECK(steady_after >= steady_before);
    CASTLE_SAMPLE_CHECK(castle::chrono::system_clock::to_time_t(converted) == static_cast<time_t>(2));
    CASTLE_SAMPLE_CHECK(castle::chrono::high_resolution_clock::to_time_t(
        castle::chrono::high_resolution_clock::from_time_t(3)) == static_cast<time_t>(3));
    (void)wall_now;
    return 0;
}
