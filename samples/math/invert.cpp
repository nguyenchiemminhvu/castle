#include "sample_support.hpp"

#include "castle/math/invert.hpp"

int main()
{
    castle::math::invert<int> negate;
    CASTLE_SAMPLE_CHECK(negate(4) == -4);
    CASTLE_SAMPLE_CHECK(negate.offset() == 0);
    CASTLE_SAMPLE_CHECK(negate.minuend() == 0);

    castle::math::invert<unsigned int> reflect;
    CASTLE_SAMPLE_CHECK(reflect(0U) == castle::numeric_limits<unsigned int>::max());

    castle::math::invert<unsigned int> custom(10U, 50U);
    CASTLE_SAMPLE_CHECK(custom(10U) == 50U);
    CASTLE_SAMPLE_CHECK(custom(60U) == 0U);
    return 0;
}
