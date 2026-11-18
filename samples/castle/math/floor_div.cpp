#include "sample_support.hpp"

#include "castle/math/floor_div.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::floor_div(7, 3) == 2);
    CASTLE_SAMPLE_CHECK(castle::math::floor_div(6, 3) == 2);
    CASTLE_SAMPLE_CHECK(castle::math::floor_div(-7, 3) == -3);
    CASTLE_SAMPLE_CHECK(castle::math::floor_div(0, 3) == 0);
    return 0;
}
