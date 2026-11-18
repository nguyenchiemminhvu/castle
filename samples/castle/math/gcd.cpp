#include "sample_support.hpp"

#include "castle/math/gcd.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK((castle::math::gcd<84, 30>::value == 6));
    CASTLE_SAMPLE_CHECK((castle::math::gcd<0, 9>::value == 9));
    CASTLE_SAMPLE_CHECK((castle::math::gcd<0, 0>::value == 0));
    return 0;
}
