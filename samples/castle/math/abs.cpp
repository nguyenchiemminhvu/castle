#include "sample_support.hpp"

#include "castle/math/abs.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::abs(-42) == 42);
    CASTLE_SAMPLE_CHECK(castle::math::abs(42U) == 42U);
    CASTLE_SAMPLE_CHECK(castle::math::abs(-3.5f) == 3.5f);
    CASTLE_SAMPLE_CHECK(castle::math::uabs(-42) == 42U);

    using unsigned_int = typename castle::meta::make_unsigned<int>::type;
    const unsigned_int min_magnitude =
        castle::math::uabs(castle::numeric_limits<int>::min());
    const unsigned_int expected =
        static_cast<unsigned_int>(
            (castle::numeric_limits<unsigned_int>::max() /
             static_cast<unsigned_int>(2)) +
            static_cast<unsigned_int>(1));

    CASTLE_SAMPLE_CHECK(min_magnitude == expected);
    return 0;
}
