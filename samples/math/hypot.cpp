#include "sample_support.hpp"

#include "castle/math/hypot.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::hypot(3.0f, 4.0f) == 5.0f);
    CASTLE_SAMPLE_CHECK(castle::math::hypot(1.0f, 2.0f, 2.0f) == 3.0f);
    CASTLE_SAMPLE_CHECK(castle::math::hypot(0.0f, 0.0f) == 0.0f);
    return 0;
}
