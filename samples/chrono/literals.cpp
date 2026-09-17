#include "sample_support.hpp"

#include "castle/chrono/literals.hpp"

int main()
{
    using namespace castle::chrono::literals::chrono_literals;
    const auto ns = 1_ns;
    const auto us = 2_us;
    const auto ms = 3_ms;
    const auto s = 4_s;
    const auto min = 5_min;
    const auto h = 6_h;
    const auto d = 7_d;
    const auto w = 8_w;

    CASTLE_SAMPLE_CHECK(ns.count() == 1);
    CASTLE_SAMPLE_CHECK(us.count() == 2);
    CASTLE_SAMPLE_CHECK(ms.count() == 3);
    CASTLE_SAMPLE_CHECK(s.count() == 4);
    CASTLE_SAMPLE_CHECK(min.count() == 5);
    CASTLE_SAMPLE_CHECK(h.count() == 6);
    CASTLE_SAMPLE_CHECK(d.count() == 7);
    CASTLE_SAMPLE_CHECK(w.count() == 8);
    CASTLE_SAMPLE_CHECK((ms + 2_ms).count() == 5);
    return 0;
}
