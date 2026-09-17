#include "sample_support.hpp"

#include "castle/math/fib.hpp"

int main()
{
    CASTLE_SAMPLE_CHECK(castle::math::fib<0U>() == static_cast<castle::size_type>(0U));
    CASTLE_SAMPLE_CHECK(castle::math::fib<1U>() == static_cast<castle::size_type>(1U));
    CASTLE_SAMPLE_CHECK(castle::math::fib<10U>() == static_cast<castle::size_type>(55U));
    CASTLE_SAMPLE_CHECK(castle::math::fib(0U) == static_cast<castle::size_type>(0U));
    CASTLE_SAMPLE_CHECK(castle::math::fib(1U) == static_cast<castle::size_type>(1U));
    CASTLE_SAMPLE_CHECK(castle::math::fib(10U) == static_cast<castle::size_type>(55U));
    return 0;
}
