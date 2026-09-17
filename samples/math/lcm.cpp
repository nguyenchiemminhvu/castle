#include "sample_support.hpp"

#include "castle/math/lcm.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK((castle::math::lcm<12, 15>::value == 60));
    CASTLE_SAMPLE_CHECK((castle::math::lcm<1, 9>::value == 9));
    return 0;
}
