#include "sample_support.hpp"

#include "castle/math/is_power_of_two.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(!castle::math::is_power_of_two(0U));
    CASTLE_SAMPLE_CHECK(castle::math::is_power_of_two(1U));
    CASTLE_SAMPLE_CHECK(castle::math::is_power_of_two(16U));
    CASTLE_SAMPLE_CHECK(!castle::math::is_power_of_two(12U));
    return 0;
}
