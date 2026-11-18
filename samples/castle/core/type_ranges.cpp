#include "sample_support.hpp"

#include "castle/core/type_ranges.hpp"

int main()
{
    static_assert(castle::numeric_limits<bool>::is_specialized, "bool specialization");
    static_assert(castle::numeric_limits<int>::is_signed, "int signed");
    static_assert(castle::numeric_limits<unsigned int>::is_integer, "unsigned integer");
    static_assert(castle::numeric_limits<unsigned int>::is_exact, "unsigned exact");
    static_assert(castle::numeric_limits<float>::is_specialized, "float specialization");
    static_assert(!castle::numeric_limits<float>::is_integer, "float is not integer");

    CASTLE_SAMPLE_CHECK(castle::numeric_limits<unsigned char>::min() == 0U);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<unsigned char>::max() >= 255U);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<int>::lowest() < 0);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<long long>::max() > 0);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<bool>::min() == false);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<bool>::max() == true);
    CASTLE_SAMPLE_CHECK(castle::numeric_limits<double>::lowest() < 0.0);
    return 0;
}
