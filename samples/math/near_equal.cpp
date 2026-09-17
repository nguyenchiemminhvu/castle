#include "sample_support.hpp"

#include "castle/math/near_equal.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(1000.0f, 1000.5f, 0.001f));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(0.0004f, 0.0f, 0.001f));
    CASTLE_SAMPLE_CHECK(!castle::math::near_equal(1.0f, 1.01f, 0.001f));
    return 0;
}
