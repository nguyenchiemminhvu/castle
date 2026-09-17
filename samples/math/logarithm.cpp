#include "sample_support.hpp"

#include "castle/math/logarithm.hpp"

int main()
{
    static_assert(castle::math::logarithm<0U, 2U>::value == 0U, "");
    static_assert(castle::math::logarithm<81U, 3U>::value == 4U, "");
    static_assert(castle::math::log2<1024U>::value == 10U, "");
    static_assert(castle::math::log10<999U>::value == 2U, "");

    CASTLE_SAMPLE_CHECK((castle::math::logarithm<15U, 2U>::value == 3U));
    return 0;
}
