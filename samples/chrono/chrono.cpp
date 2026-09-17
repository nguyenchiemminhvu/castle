#include "sample_support.hpp"

#include "castle/chrono/chrono.hpp"

int main()
{
    using namespace castle::chrono::literals::chrono_literals;

    const auto timeout = 250_ms;
    const auto start = castle::chrono::steady_clock::now();
    const auto deadline = start + timeout;
    const auto wall = castle::chrono::system_clock::from_time_t(4);

    CASTLE_SAMPLE_CHECK(timeout.count() == 250);
    CASTLE_SAMPLE_CHECK(deadline >= start);
    CASTLE_SAMPLE_CHECK(castle::chrono::system_clock::to_time_t(wall) == static_cast<time_t>(4));
    return 0;
}
