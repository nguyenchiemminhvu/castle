#include "sample_support.hpp"

#include "castle/math/near_equal.hpp"
#include "castle/math/numeric/result.hpp"

int main()
{
    // Converged record.
    const castle::math::numeric_result<double> converged = {1.0, 1e-9, 5U, castle::status::ok};
    CASTLE_SAMPLE_CHECK(converged.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(converged.value, 1.0, 1e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(converged.error, 1e-9, 1e-12));
    CASTLE_SAMPLE_CHECK(converged.iterations == 5U);
    CASTLE_SAMPLE_CHECK(converged.code == castle::status::ok);

    // Budget exhausted: best estimate is still present.
    const castle::math::numeric_result<float> exhausted = {2.5F, 0.1F, 12U, castle::status::out_of_range};
    CASTLE_SAMPLE_CHECK(!exhausted.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(exhausted.value, 2.5F, 1e-6F));

    // Unusable input.
    const castle::math::numeric_result<double> invalid = {0.0, 0.0, 0U, castle::status::invalid_argument};
    CASTLE_SAMPLE_CHECK(!invalid.ok());

    // Mixed absolute/relative tolerance used by the iterative algorithms.
    CASTLE_SAMPLE_CHECK(castle::math::detail::within_tolerance(1e-9, 0.5, 1e-8));
    CASTLE_SAMPLE_CHECK(!castle::math::detail::within_tolerance(1e-7, 0.5, 1e-8));
    CASTLE_SAMPLE_CHECK(castle::math::detail::within_tolerance(5e-5, 1000.0, 1e-7));
    CASTLE_SAMPLE_CHECK(!castle::math::detail::within_tolerance(5e-3, 1000.0, 1e-7));
    CASTLE_SAMPLE_CHECK(castle::math::detail::within_tolerance(5e-5F, -1000.0F, 1e-7F));

    constexpr bool compile_time = castle::math::detail::within_tolerance(0.0, 0.0, 0.0);
    CASTLE_SAMPLE_CHECK(compile_time);
    return 0;
}
