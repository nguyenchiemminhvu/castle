#include "sample_support.hpp"

#include "castle/math/mod.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::positive_mod(-1, 5) == 4);
    CASTLE_SAMPLE_CHECK(castle::math::positive_mod(14, 5) == 4);
    CASTLE_SAMPLE_CHECK(castle::math::wrap(14, 10, 13) == 11);
    CASTLE_SAMPLE_CHECK(castle::math::wrap(-1, 0, 5) == 4);
    CASTLE_SAMPLE_CHECK(castle::math::wrap(25, -2, 3) == 0);
    return 0;
}
