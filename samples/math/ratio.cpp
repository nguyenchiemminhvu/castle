#include "sample_support.hpp"

#include "castle/math/ratio.hpp"

int main()
{
    using half = castle::math::ratio<2, 4>;
    using third = castle::math::ratio<1, 3>;
    using sum = castle::math::ratio_add_t<half, third>;
    using product = castle::math::ratio_multiply_t<half, third>;

    static_assert(half::num == 1 && half::den == 2, "");
    static_assert(sum::num == 5 && sum::den == 6, "");
    static_assert(product::num == 1 && product::den == 6, "");
    static_assert(castle::math::ratio_less<third, half>::value, "");
    static_assert(castle::milli::num == 1 && castle::milli::den == 1000, "");

    CASTLE_SAMPLE_CHECK(sum::num == 5 && sum::den == 6);
    return 0;
}
