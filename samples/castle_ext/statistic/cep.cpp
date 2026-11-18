#include "sample_support.hpp"

#include <stdint.h>

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/cep.hpp"

int main()
{
    // GNSS accuracy report: keep the latest 100 horizontal errors, drop the
    // oldest one for every new fix, and report CEP50/68/95/99 in metres.
    castle::statistic::exact_cep<float, 100U> horizontal;
    CASTLE_SAMPLE_CHECK(horizontal.empty());

    float value = 0.0f;
    CASTLE_SAMPLE_CHECK(horizontal.cep95(value) == castle::status::empty);

    // 150 fixes with radial errors 0.01 m, 0.02 m, ... 1.50 m. Only the newest
    // 100 (0.51 m ... 1.50 m) are retained.
    for (uint32_t i = 1U; i <= 150U; ++i)
    {
        CASTLE_SAMPLE_CHECK(horizontal.push_distance(0.01f * static_cast<float>(i)) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(horizontal.count() == 100U);

    // The retained errors are 0.51 ... 1.50, so the linear quantiles are:
    CASTLE_SAMPLE_CHECK(horizontal.cep50(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 1.005f) < 1e-3f);
    CASTLE_SAMPLE_CHECK(horizontal.cep68(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 1.1832f) < 1e-3f);
    CASTLE_SAMPLE_CHECK(horizontal.cep95(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 1.4505f) < 1e-3f);
    CASTLE_SAMPLE_CHECK(horizontal.cep99(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 1.4901f) < 1e-3f);

    // Half of the retained fixes (0.51 m ... 1.00 m) lie within 1.002 m.
    CASTLE_SAMPLE_CHECK(horizontal.fraction_within(1.002f, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.5f) < 1e-3f);

    // East/north residuals against a surveyed point become radii: 3-4-5.
    horizontal.clear();
    CASTLE_SAMPLE_CHECK(horizontal.push_residual(3.0f, 4.0f) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(horizontal.cep50(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 5.0f) < 1e-4f);

    // Invalid errors are rejected.
    CASTLE_SAMPLE_CHECK(horizontal.push_distance(-0.1f) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(horizontal.percentile(101.0f, value) == castle::status::invalid_argument);

    // Whole-stream CEP in O(1) memory: a histogram whose range grows on demand.
    castle::statistic::cumulative_cep<double, 8U> drive(0.0, 8.0);
    CASTLE_SAMPLE_CHECK(drive.push_distance(1.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(drive.push_distance(2.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(drive.push_distance(20.0) == castle::status::ok);
    double radius = 0.0;
    CASTLE_SAMPLE_CHECK(drive.cep50(radius) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(radius - 3.0) < 1e-9);

    return 0;
}
