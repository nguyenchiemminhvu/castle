#include "sample_support.hpp"

#include "castle/math/powi.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::powi(5, 0U) == 1);
    CASTLE_SAMPLE_CHECK(castle::math::powi(2U, 10U) == 1024U);
    CASTLE_SAMPLE_CHECK(castle::math::powi(-3, 3U) == -27);
    return 0;
}
