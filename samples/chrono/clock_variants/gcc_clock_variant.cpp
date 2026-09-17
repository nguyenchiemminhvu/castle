#include "sample_support.hpp"

#include "castle/chrono/clock_variants/gcc_clock_variant.hpp"

int main()
{
    const timespec realtime = castle::chrono::detail::clock_variant::realtime_ns();
    const timespec monotonic = castle::chrono::detail::clock_variant::monotonic_ns();

    CASTLE_SAMPLE_CHECK(realtime.tv_nsec >= 0 && realtime.tv_nsec < 1000000000L);
    CASTLE_SAMPLE_CHECK(monotonic.tv_nsec >= 0 && monotonic.tv_nsec < 1000000000L);
    return 0;
}
