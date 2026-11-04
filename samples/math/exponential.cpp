#include "sample_support.hpp"

#include "castle/math/exponential.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    // float overloads
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::exp(0.0F), 1.0F, 1.0e-6F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::exp(1.0F), 2.7182818F, 1.0e-6F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::log(1.0F), 0.0F, 1.0e-6F));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::log(2.7182818F), 1.0F, 1.0e-6F));

    // double overloads
    const double e1 = castle::math::exp(1.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(e1, 2.718281828459045, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::log(e1), 1.0, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::exp(-1.0), 1.0 / e1, 1.0e-12));

    // long double overloads
    const long double e1l = castle::math::exp(1.0L);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(e1l, 2.718281828459045L, 1.0e-12L));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::log(e1l), 1.0L, 1.0e-12L));

    return 0;
}
