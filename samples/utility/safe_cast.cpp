#include "sample_support.hpp"

#include "castle/utility/safe_cast.hpp"

#include <stdint.h>

int main()
{
    CASTLE_SAMPLE_CHECK(castle::safe_cast::bool_to_uint8(true) == 1U);
    CASTLE_SAMPLE_CHECK(castle::safe_cast::int16_to_uint8(-5) == 0U);
    CASTLE_SAMPLE_CHECK(castle::safe_cast::uint32_to_int16(70000U) == 32767);
    CASTLE_SAMPLE_CHECK(castle::safe_cast::float_to_uint8(300.0F) == 255U);
    CASTLE_SAMPLE_CHECK(castle::safe_cast::double_to_float(1.5) == 1.5F);

    CASTLE_SAMPLE_CHECK((castle::SAFE_CAST<int16_t, uint8_t>(300) == 255U));
    CASTLE_SAMPLE_CHECK((castle::SAFE_CAST<bool, double>(true) == 1.0));
    CASTLE_SAMPLE_CHECK((castle::SAFE_CAST<float, bool>(1.0F)));
    CASTLE_SAMPLE_CHECK((castle::SAFE_CAST<double, int32_t>(-12.75) == -12));
    return 0;
}
