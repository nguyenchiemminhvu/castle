#include "sample_support.hpp"

#include "castle/math/factorial.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::factorial<0U>::value ==
                        static_cast<castle::size_type>(1U));
    CASTLE_SAMPLE_CHECK(castle::math::factorial<5U>::value ==
                        static_cast<castle::size_type>(120U));
    CASTLE_SAMPLE_CHECK(castle::math::factorial_v(0U) ==
                        static_cast<castle::size_type>(1U));
    CASTLE_SAMPLE_CHECK(castle::math::factorial_v(5U) ==
                        static_cast<castle::size_type>(120U));
    return 0;
}
