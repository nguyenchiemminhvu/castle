#include "sample_support.hpp"

#include "castle/utility/macros.hpp"

#define SAMPLE_TOKEN ready

int main()
{
    const int CASTLE_CONCAT(sample_, value) = 7;
    CASTLE_SAMPLE_CHECK(sample_value == 7);
    CASTLE_SAMPLE_CHECK(CASTLE_STRINGIFY(SAMPLE_TOKEN)[0] == 'r');
    CASTLE_SAMPLE_CHECK(CASTLE_STRING(example)[0] == 'e');
    CASTLE_SAMPLE_CHECK(CASTLE_WIDE_STRING(example)[0] == L'e');
    CASTLE_SAMPLE_CHECK(CASTLE_U8_STRING(example)[0] == 'e');
    CASTLE_SAMPLE_CHECK(CASTLE_U16_STRING(example)[0] == u'e');
    CASTLE_SAMPLE_CHECK(CASTLE_U32_STRING(example)[0] == U'e');
    CASTLE_SAMPLE_CHECK(CASTLE_BIT(5U) == 32U);
    CASTLE_SAMPLE_CHECK(CASTLE_BIT64(9U) == 512ULL);
    return 0;
}
